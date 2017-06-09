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
  if (array_key_exists('error', $library)) {
    drupal_set_message($library['error message'], 'error');
    return FALSE;
  }
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
  // for simple url w/port ie. localhost:8080
  else if(preg_match('/(.*)\:(\d+)\/*$/', $url, $matches)) {
    $use_https = FALSE;
    $host = $matches[1];
    $port = $matches[2];
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
  $all_ok = FALSE;

  // XXX: There needs to be an error check here
  // If there's an altered workflow this will throw an error
  // Need to go further up to find out why this is being called and not
  // getting an invocation
  foreach ($invocation['steps'] as $step) {
    $step_index = $step['order_index'];

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
    if ($state['step'] != 'null' or $state['step'] != 'ok') {
      $istate = 'ok';
    }
    $state[$step_index] = array(
      'step_name' => $step_name,
      'step_id' => $step['id'],
      'job_id' => $step['job_id'],
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
  if (!$status) {
    $status = 'Running';
  }
  switch ($istate) {
    case 'error':
      $status = 'Error';
      tripal_galaxy_send_submission_failed_mail($sid, $node->nid);
      break;
    case 'ok':
      $status = 'Completed';
      tripal_galaxy_send_submission_ended_mail($sid, $node->nid);
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
 * Invokes all submitted workflows that are in the 'Waiting' state.
 *
 *
 */
function tripal_invoke_galaxy_workflow($sid) {

  // Get the workflows that need execution
  $query = db_select('tripal_galaxy_workflow_submission', 'tgws');
  $query->fields('tgws');
  $query->join('tripal_galaxy_workflow', 'tgw', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
  $query->fields('tgw', array('galaxy_id', 'workflow_id', 'nid'));
  $query->condition('tgws.sid', $sid);
  $submission = $query->execute()->fetchObject();

  // Get a connection to the remote galaxy instance.
  $galaxy = tripal_get_galaxy_connection($submission->galaxy_id);
  if (!$galaxy) {
    $error = $galaxy->getError();
    throw new Exception('Cannot connect to Galaxy server:' . $error['message']);
  }

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

  // Get the history that we'll use for this submission.
  $error = array();
  $history = tripal_get_galaxy_history($galaxy, "TG-" . $user->uid . "-" . $submission->galaxy_workflow_id . "-" . $submission->sid . '-' . time(), $error);
  if (!$history) {
    $error = $galaxy->getError();
    throw new Exception($error['message']);
  }

  // Get the history contents so we don't upload the same file more than once
  // in the event that this invokation occurs more than once.
  $ghistory_contents = new GalaxyHistoryContents($galaxy);
  $hcontents = $ghistory_contents->index(array('history_id' => $history['id']));
  $error = $galaxy->getError();
  if ($error['message']) {
    throw new Exception($error['message']);
  }

  $upload_files = array();
  foreach ($hcontents as $file) {
    if (!$file['deleted'] and $file['state'] == 'ok') {
      $upload_files[] = $file['name'];
    }
  }

  // Upload any files that need uploading before submitting. These files are
  // in the file_usage table, managed by the webform module and associated
  // with this submission id (node ID).
  $files = db_select('file_usage', 'fu')
    ->fields('fu', array('fid'))
    ->condition('module', 'tripal_galaxy')
    ->condition('id', $submission->nid)
    ->execute();
  $managed_files = array();
  while ($file = $files->fetchObject()) {
    $fid = $file->fid;
    $file = file_load($fid);
    $managed_files[$fid] = $file;
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

  // Next we need to re-get the history content to get the IDs of the new files.
  $hcontents = $ghistory_contents->index(array('history_id' => $history['id']));
  $input_datasets = array();

  // Iterate through the submitted data values and create an input aray
  // for Galaxy workflow submission.
  while($data = $submitted_data->fetchObject()) {
    $value = '';

    // The component 'extra' data is serialized so we need to convert it.
    $data->extra = unserialize($data->extra);

    // Get some step details.
    $step_index = $data->extra['step_index'];
    $input_name = $data->extra['input_full_name'];

    // Skip components that don't have an 'input_name' field as these
    // don't correspond to workflow input components (e.g. fieldsets, and
    // page separators).
    if (!array_key_exists('input_name', $data->extra)) {
      continue;
    }

    // Input elements of type 'fixed_value' should not be provided as
    // input for the Galaxy workflow because the defaults should be used
    if ($data->type == 'fixed_value') {
      $value = $data->data;
      if (array_key_exists('linked', $data->extra) and $data->extra['linked']) {
        list($source_step, $step_output) = explode('|', $data->data);
        $value = array(
          'step_output' => $step_output,
          'source_step' => $source_step,
        );
      }
    }

    // If this component is a 'file' then we need to set the value to the
    // dataset ID.
    else if ($data->type == 'file') {
    $file = $managed_files[$data->data];
      foreach ($hcontents as $hcontent) {
        if ($hcontent['name'] == $file->filename) {
          $value = $hcontent['id'];
          $input_datasets[$step_index] = array(
            'id' => $value,
            'src' => 'hda',
          );
        }
      }
    }
    else if ($data->type == 'html5_file') {
      $file = $managed_files[$data->data];
      foreach ($hcontents as $hcontent) {
        if ($hcontent['name'] == $file->filename) {
          $value = $hcontent['id'];
          $input_datasets[$step_index] = array(
            'id' => $value,
            'src' => 'hda',
          );
        }
      }
    }
    // Deal with the BDSS_file type.  It will have two rows in the
    // submitted data array. One for file_URL and one for file_upload.
    // they are mutually exclusive and only one will have a real value
    else if ($data->type == 'BDSS_file') {
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

    if (!array_key_exists($step_index, $parameters)) {
      $parameters[$step_index] = array();
    }
    $current_values = $parameters[$step_index];
    _tripal_galaxy_workflow_invoke_build_values($current_values, $input_name, $value);
    $parameters[$step_index] = $current_values;
  }

   //print_r($parameters);
   //print_r($input_datasets);


  // Invoke the workflow and check for errors
  $gworkflows = new GalaxyWorkflows($galaxy);
  $invocation = $gworkflows->invoke(array(
    'workflow_id' => $submission->workflow_id,
    'parameters' => $parameters,
    'inputs' => $input_datasets,
    'history_id' => $history['id'],
  ));

  if (!$invocation) {
    $error = $galaxy->getError();
    drupal_set_message($error['message'], 'error');
    db_update('tripal_galaxy_workflow_submission')
      ->fields(array(
        'status' => 'Error',
        'errors' => $error['message'],
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
 * Retrieves the current history from Galaxy.
 *
 * @param $galaxy
 *   A GalaxyInstance object
 * @param $error
 *   An empty array into which the error type and message will be placed
 *   if an error occurs.
 *
 * @return
 *   A history array for the specified history.  If a failure occured then
 *   FALSE is returned and the $error argument is set.
 */
function tripal_get_current_history(GalaxyInstance $galaxy, &$error) {
  $ghistories = new GalaxyHistories($galaxy);
  $current_history = $ghistories->getMostRecentlyUsed();

  if (!$current_history){
    $error = $galaxy->getError();
    return FALSE;
  }

  return $current_history;
}

/**
 * Retrieves a history by name from Galaxy.
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
  if (!$history){
    $error = $galaxy->getError();
    return FALSE;
  }
  return $history;
}

/**
 * Tests if a Galaxy server is accessible.
 *
 * @param $connect
 *   An array of the following:
 *    - galaxy_id: A unique ID for the galaxy server. If this is provided no
 *        other arguments are needed.
 *    - host: The DNS hostname of the galaxy server.
 *    - port: The TCP port for the server.
 *    - use_https:  Set to TRUE of the server uses HTTPS
 *
 * @param
 *   Returns TRUE if accessible. FALSE otherwise.  A Drupal message is
 *   also provided that indicates if the test was successful.
 */
function tripal_test_galaxy_connection($connect) {

  $library = libraries_load('blend4php');
  if (array_key_exists('error', $library)) {
    drupal_set_message($library['error message'], 'error');
    return FALSE;
  }

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

/**
 * Generates the contents of the current history from the Galaxy server.
 *
 *  The first element in the array should be the entire history compressed
 *  as a single file.
 *  'entire' => [url_to_download_whole_history]
 * @param $galaxy
 *   A GalaxyInstance object
 * @param $error
 *   An empty array into which the error type and message will be placed
 *   if an error occurs.
  *
 * @param
 *   Returns an associative array with the keys being the name of the step and
 *   the values being the URLs of the results of that particular step.
 */
 function tripal_get_current_galaxy_history_download_links(GalaxyInstance $galaxy, &$error) {

   $links = array();
   $whole_history = tripal_get_current_history($galaxy, $error);

   $ghistories = new GalaxyHistories($galaxy);
   $ghistory_contents = new GalaxyHistoryContents($galaxy);

   // Acquire the individual contents from the current history.
   $contents = $ghistory_contents->index(array('history_id' => $whole_history['id']));

   if(!$contents) {
     $error = $galaxy->getError();
     return FALSE;
   }

   $history_params['url_only'] = TRUE;

   // Iterate through the contents and generate the other id's.
   $download_history_content_params = array();

   $download_history_content_params['history_id'] = $whole_history['id'];
   $download_history_content_params['url_only'] = TRUE;
   // Generate the clickable URLs.
   foreach($contents as $key => $val) {
     $download_history_content_params['history_content_id'] = $val['id'];
     // If the history content has been marked as deleted or purged DO NOT
     // include the file as a downloadable link
     if($ghistory_contents->download_history_content($download_history_content_params)){
       // Create a nested array for holding the particular job id that started
       // the creation of the history content that we are viewing.

       // API call to the specific history_content to to get the creating_job
       // id
       $links[$val['hid'] . ': ' . $val['name']] = array();
       $links[$val['hid'] . ': ' . $val['name']]['job_id'] = $ghistory_contents->show(array('id' => $val['id'], 'history_id' => $whole_history['id']))['creating_job'];
       $links[$val['hid'] . ': ' . $val['name']]['id'] = $val['id'];
       $links[$val['hid'] . ': ' . $val['name']]['link'] = $ghistory_contents->download_history_content($download_history_content_params);
     }
   }

   return $links;

 }

 /**
 *  Generate the client prompt based off of the history/history_content.
 *  TODO: Generate a variable number of the contents within a history
 *  TODO: Generate a history_content file as a compressed file.
 * @param $galaxy
 *   A GalaxyInstance object
 * @param $error
 *   An empty array into which the error type and message will be placed
 *   if an error occurs.
 * @param $history_id
 *   The history to prepare for download.
 * @param $history_content_id
 *   Optional, specify the content_id to download only that piece from the
 *   history.
 */
 function tripal_generate_history_file(GalaxyInstance $galaxy, &$error, $history_id, $history_content_id = NULL) {
   // Check if we're looking for a whole history or a single history content.
   $ghistory_contents = new GalaxyHistoryContents($galaxy);
   $params = array();
   $params['history_id'] = $history_id;
   // Download the whold history
   if(!$history_content_id) {
     return $ghistory_contents->download_history($params);
   }
   // Else download the specified content
   else {
     // Include the specified content_id
     $params['history_content_id'] = $history_content_id;
     return $ghistory_contents->download_history_content($params);
   }
 }
