<?php

/**
 * Retreives a GalaxyInstance objects using a galaxy_id
 *
 * @param $galaxy_id
 *   The ID of a galaxy server.
 *
 * @return GalaxyInstance
 *   A galaxyInstance object or FALSE on error.
 *
 */
function tripal_get_galaxy_connection($galaxy_id) {
  // Get the galaxy server for this workflow
  $galaxy_server = db_select('tripal_galaxy', 'tg')
    ->fields('tg')
    ->condition('galaxy_id', $galaxy_id)
    ->execute()
    ->fetchObject();

  $library = libraries_load('blend4php');
  $connect = tripal_split_galaxy_url($galaxy_server->url);
  $galaxy = new GalaxyInstance($connect['host'], $connect['port'], $connect['use_https']);
  $galaxy->setAPIKey($galaxy_server->api_key);
  $error = $galaxy->getErrorType();
  if ($error) {
    return FALSE;
  }
  return $galaxy;
}

/**
 * Splits a URL to a Galaxy server into the host, port and if HTTPS is required.
 *
 * @param $utl
 *   The URL for the remote galaxy instance.
 *
 * @return
 *   An array with three keys: host, port and use_https.
 */
function tripal_split_galaxy_url($url) {

  // First check a URL with a port
  $matches = array();
  if (preg_match('/^(.*)\:\/\/(.+?)\:(\d+)\/*$/', $url, $matches)) {
    $protocol = $matches[1];
    $host = $matches[2];
    $port = $matches[3];
    $use_https = FALSE;
    if ($protocol == 'https') {
      $use_https = TRUE;
    }
  }
  // Next check a URL without a port
  else if (preg_match('/^(.*)\:\/\/(.+?)\/*$/', $url, $matches)) {
    $protocol = $matches[1];
    $host = $matches[2];
    $use_https = FALSE;
    $port = 80;
    if ($protocol == 'https') {
      $use_https = TRUE;
      $port = 443;
    }
  }
  return array(
    'host' => $host,
    'port' => $port,
    'use_https' => $use_https
  );
}

/**
 * Checks and updates the status of a Galaxy workflow.
 *
 * Each Galaxy workflow has a unique UUID.  Each time the workflow is changed
 * that UUID changes as well.
 */
function tripal_check_galaxy_workflow_status($galaxy_workflow_id) {

  if (!$galaxy_workflow_id) {
    return FALSE;
  }
  if (!is_numeric($galaxy_workflow_id)) {
    return FALSE;
  }
  $workflow = db_select('tripal_galaxy_workflow', 'tgw')
    ->fields('tgw')
    ->condition('tgw.galaxy_workflow_id', $galaxy_workflow_id)
    ->execute()
    ->fetchObject();

  // Connect to the Galaxy instance and get the list of workflows
  $galaxy = tripal_get_galaxy_connection($workflow->galaxy_id);
  $workflows = new GalaxyWorkflows($galaxy);
  $gworkflow = $workflows->show(array('workflow_id' => $workflow->workflow_id));

  // If the UUIDs don't match then someone changed the workflow and it
  // is no longer guaranteed to be the same. So, we'll mark it as 'Altered'.
  if ($gworkflow['latest_workflow_uuid'] != $workflow->workflow_uuid) {
    db_update('tripal_galaxy_workflow')
      ->fields(array(
        'status' => 'Altered'
      ))
      ->condition('galaxy_workflow_id', $workflow->galaxy_workflow_id)
      ->execute();
  }

  return TRUE;
}

/**
 * Checks and updates the status of a Galaxy workflow.
 *
 * Each Galaxy workflow has a unique UUID.  Each time the workflow is changed
 * that UUID changes as well.
 */
function tripal_check_galaxy_submission_status($sid) {

  if (!$sid) {
    return FALSE;
  }
  if (!is_numeric($sid)) {
    return FALSE;
  }

  $query = db_select('tripal_galaxy_workflow_submission', 'tgws');
  $query->fields('tgws', array('invocation_id', 'galaxy_workflow_id'));
  $query->join('tripal_galaxy_workflow', 'tgw', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
  $query->fields('tgw', array('galaxy_id', 'workflow_id', 'nid'));
  $query->condition('tgws.sid', $sid);
  $submission = $query->execute()->fetchObject();

  dpm($submission);

  // Get the node for this submission.
  $node = node_load($submission->nid);

  // Connect to the Galaxy instance and get the list of workflows
  $galaxy = tripal_get_galaxy_connection($submission->galaxy_id);
  $gworkflows = new GalaxyWorkflows($galaxy);
  $error = $galaxy->getError();

  // Get the invocation specified.
  $invocation = $gworkflows->showInvocations(array(
    'workflow_id' => $submission->workflow_id,
    'invocation_id' => $submission->invocation_id
  ));
  $error = $galaxy->getError();

  // Get the overall invocation state.
  $istate = $invocation['state'];

  // Iterate through the invocation steps and look for any steps that
  // are in the error state.
  $state = array();
  $has_error = FALSE;
  foreach ($invocation['steps'] as $step_index => $step) {
    // Get the STDERR and STDOUT for the job.
    $gjobs = new GalaxyJobs($galaxy);
    $job = $gjobs->show(array(
      'job_id' => $step['job_id'],
      'full' => TRUE,
    ));
    $stderr = $job['stderr'];
    $stdout = $job['stdout'];
    $misc_info = '';

    // Get the tool
    $gtools = new GalaxyTools($galaxy);
    $tool = $gtools->show(array('tool_id' => $job['tool_id']));
    $step_name = 'Step ' . ($step_index + 1). ': ' . $tool['name'] . ' v' . $tool['version'];

    if ($step['state'] == 'error') {

      // Sometimes the error message is in a dataset. So, get the 'misc_info'
      // from that.
      $gdataset = new GalaxyDatasets($galaxy);
      $dataset = $gdataset->show(array('dataset_id' => $job['outputs']['output']['id']));
      $misc_info = $dataset['misc_info'];;
      $has_error = TRUE;
    }
    $state[$step_index] = array(
      'step_name' => $step_name,
      'state' => $job['state'],
      'stderr' => $stderr,
      'stdout' => $stdout,
      'misc_info' => $misc_info,
    );

  }

  if ($has_error) {
    $istate = 'error';
  }

  $status = $submission->status;
  switch ($istate) {
    case 'error':
      $status = 'Error';
      tripal_galaxy_send_submission_failed_mail($sid, $node->nid);
      break;
    case 'ok':
      $status = 'Completed';
      tripal_galaxy_send_submission_ended_mail($sid, $node->nid);
      break;
    case 'running':
      $status = 'Running';
      break;
  }

  db_update('tripal_galaxy_workflow_submission')
    ->fields(array(
      'status' => $status,
      'errors' => serialize($state)
    ))
    ->condition('sid', $sid)
    ->execute();
  return TRUE;
}

/**
 * Invokes all submitted workflwos that are in the 'Waiting' state.
 *
 */
function tripal_invoke_galaxy_workflow($sid){

  global $user;

  // Get the workflows that need execution
  $query = db_select('tripal_galaxy_workflow_submission', 'tgws');
  $query->fields('tgws');
  $query->join('tripal_galaxy_workflow', 'tgw', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
  $query->fields('tgw', array('galaxy_id', 'workflow_id', 'nid'));
  $query->condition('tgws.sid', $sid);
  $submission = $query->execute()->fetchObject();

  // The node for this submission
  $node = node_load($submission->nid);

  // The array where the workflow input parameters are stored.
  $parameters = array();

  $query = db_select('webform_submitted_data', 'wsd');
  $query->fields('wsd');
  $query->join('webform_component', 'wc', 'wc.cid = wsd.cid');
  $query->fields('wc', array('type', 'extra'));
  $query->condition('wsd.sid', $submission->sid);
  $query->orderBy('wsd.cid');
  $submitted_data = $query->execute();

  // Get a connection to the remote galaxy instanc4e.
  $galaxy = tripal_get_galaxy_connection($submission->galaxy_id);
  if (!$galaxy) {
    $error = $galaxy->getError();
    throw new Exception($error['message']);
  }

  // Get the history that we'll use for this submission.
  $error = array();
  $history = tripal_get_galaxy_history($galaxy, "TG-" . $user->uid . "-" . $submission->galaxy_workflow_id . "-" . $submission->sid, $error);
  if (!$history) {
    $error = $galaxy->getError();
    throw new Exception($error['message']);
  }

  // Get the history contents so we don't upload the same file more than once
  // in the event that this invokation occurs more than once.
  $ghistory_contents = new GalaxyHistoryContents($galaxy);
  $hcontents = $ghistory_contents->index(array('history_id' => $history['id']));
  if (!$hcontents) {
    $error = $galaxy->getError();
    throw new Exception($error['message']);
  }
  $upload_files = array();
  foreach ($hcontents as $file) {
    if ($file['deleted'] != 1) {
      $upload_files[] = $file['name'];
    }
  }

  // Upload any files that need uploading before submitting. These files are
  // in the file_usage table, managed by the webform module and associated
  // with this submission id (node ID).
  $files = db_select('file_usage', 'fu')
    ->fields('fu', array('fid'))
    ->condition('module', 'webform')
    ->condition('type', 'submission')
    ->condition('id', $sid)
    ->execute();

  while ($file = $files->fetchObject()) {
    $fid = $file->fid;
    $file = file_load($fid);
    // Only upload the file if it isn't already there.
    if (!in_array($file->filename, $upload_files)) {
      $file_list = array();
      $file_list[] = array(
        'name' => $file->filename,
        'path' =>  drupal_realpath($file->uri)
      );
      // Now upload the files.
      $gtool = new GalaxyTools($galaxy);
      $tool = $gtool->create(array(
        'tool_id' => 'upload1',
        'history_id' => $history['id'],
        'files' => $file_list,
      ));
      if (!$tool) {
        $error = $galaxy->getError();
        throw new Exception($error['message']);
      }
    }
  }

  // Iterate through the submitted data values and create an input aray
  // for Galaxy workflow submission.
  while($data = $submitted_data->fetchObject()) {
    $value = '';

    // The component 'extra' data is serialized so we need to convert it
    $data->extra = unserialize($data->extra);

    // Skip components that don't have an 'input_name' field as these
    // don't correspond to workflow input components.
    if (!array_key_exists('input_name', $data->extra)) {
      continue;
    }

    // Input elements of type 'fixed_value' should not be provided as
    // input for the Galaxy workflow because the defaulst should be used
    if ($data->type == 'fixed_value') {
      continue;
    }


    // Deal with the BDSS_file type.  It will have two rows in the
    // submitted data array. One for file_URL and one for file_upload.
    // they are mutually exclusive and only one will have a real value
    if ($data->type == 'BDSS_file') {
      if ($data->no == 'file_URL' and $data->data) {
        $value = $data->data;
      }
      if ($data->no == 'file_upload' and $data->data) {
        $fid = $data->data;
        if ($fid) {
          $file = file_load($fid);
          if ($file) {
            $value = file_create_url($file->uri);
          }
        }
      }
    }
    else {
      $value = $data->data;
    }

    // If we don't have a value then skip it and let the workflow use it's
    // own defaults
    if (!$value) {
      continue;
    }

    // NOTE: The step id's (page_num) on the server side are offset by 1
    // so subtract 1, then organize the input settings by their step in the
    // $galaxy_input['parameters'] array.
    $step_index = $data->extra['step_index'];
    $input_name = $data->extra['input_full_name'];
    if (!array_key_exists($step_index, $parameters)) {
      $parameters[$step_index] = array();
    }
    $current_values = $parameters[$step_index];
    _tripal_galaxy_workflow_invoke_build_values($current_values, $input_name, $value);
    $parameters[$step_index] = $current_values;
  }


  // Invoke the workflow and check for errors
  $gworkflows = new GalaxyWorkflows($galaxy);
//     $invocation = $gworkflows->invoke(array(
//       'workflow_id' => $submission->workflow_id,
//       'parameters' => $parameters,
//       'input_dataset_ids' => array(),
//       'hist_id' => $history_id,
//     ));
return;
  $error = $galaxy->getError();
  if ($error['type']) {
    watchdog_exception($error['message'], 'error');
    db_update('tripal_galaxy_workflow_submission')
      ->fields(array(
        'status' => 'ERROR',
        'error' => $error['message'],
      ))
      ->condition('sid', $submission->sid)
      ->execute();
    tripal_galaxy_send_submission_failed_mail($submission->sid, $node->uid);
  }
  else {
    db_update('tripal_galaxy_workflow_submission')
      ->fields(array(
        'status' => 'Submitted',
        'start_time' => REQUEST_TIME,
        'invocation_id' => $invocation['id'],
      ))
      ->condition('sid', $submission->sid)
      ->execute();
    tripal_galaxy_send_submission_start_mail($submission->sid, $node->uid);
  }

}

/**
 * Retrieves a specific history for Galaxy.
 *
 * @param $galaxy
 *   A GalaxyInstance object
 * @param $history_name
 *   The name of the history to retrieve. If the history doesn't exist then
 *   it will be created.
 * @param $error
 *   An empty array into which the error type and message will be placed
 *   if an error occurs.
 *
 * @return
 *   A history array for the specified history.  If a failure occured then
 *   FALSE is returned and the $error argument is set.
 */
function tripal_get_galaxy_history(GalaxyInstance $galaxy, $history_name, &$error) {
  $ghistories = new GalaxyHistories($galaxy);

  // Look through existing histories to find what we're looking for.
  $histories = $ghistories->index();
  foreach ($histories as $history) {
    if ($history['name'] == $history_name) {
      return $history;
    }
  }

  // If we're here then the history doesn't exist, so create one.
  $history = $ghistories->create(array(
    'name' => $history_name,
  ));
  dpm($history);
  if (!$history){
    $error = $galaxy->getError();
    return FALSE;
  }
}

/**
 * Tests if a Galaxy server is accessible.
 *
 * @param $connect
 *   An array of the following:
 *    - galaxy_id:  A unique ID for the galaxy server. If this is provided no
 *        other arguments are needed.
 *    - host:  The DNS hostname of the galaxy server.
 *    - port:  The TCP port for the server.
 *    - use_https:  Set to TRUE of the server uses HTTPS
 *
 * @param
 *   Returns TRUE if accessible. FALSE otherwise.  A Drupal message is
 *   also provided that indicates if the test was successful.
 */
function tripal_test_galaxy_connection($connect) {

  if (array_key_exists('galaxy_id', $connect)) {
    $galaxy_id = $connect['galaxy_id'];
    $galaxy = tripal_get_galaxy_connection($galaxy_id);
    $error = $galaxy->getError();
    if ($error['message']) {
      drupal_set_message('Could not connect: ' . $error['message'], 'error');
      return FALSE;
    }
  }
  else {
    // Create a new galaxy instance using the obtained hostname and port
    // Then Authenticate
    $library = libraries_load('blend4php');
    $galaxy = new GalaxyInstance($connect['host'], $connect['port'], $connect['use_https']);
    $error = $galaxy->getError();
    if ($error['message']) {
      drupal_set_message('Could not connect: ' . $error['message'], 'error');
      return FALSE;
    }
  }

  // Tell the user whether the connection was successful based on
  // getVersion()
  $version = $galaxy->getVersion();
  if ($version == NULL) {
    $error = $galaxy->getError();
    drupal_set_message('Could not connect: ' . $error['message'], 'error');
    return FALSE;
  }
  else {
    drupal_set_message(t('Successful connection to the Galaxy server (version %version)', array('%version' => $version['version_major'])));
  }
  return TRUE;
}