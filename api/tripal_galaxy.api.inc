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
  if (preg_match('/^(.*)\:\/\/(.+?)\:(\d+)$/', $url, $matches)) {
    $protocol = $matches[1];
    $host = $matches[2];
    $port = $matches[3];
    $use_https = FALSE;
    if ($protocol == 'https') {
      $use_https = TRUE;
    }
  }
  // Next check a URL without a port
  else if (preg_match('/^(.*)\:\/\/(.+?)$/', $url, $matches)) {
    $protocol = $matches[1];
    $host = $matches[2];
    $port = '';
    $use_https = FALSE;
    if ($protocol == 'https') {
      $use_https = TRUE;
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
  $submission = $query->execute()->fetchObject;

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
          $value = file_create_url($file->uri);
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

  // Get a connection to the remote galaxy instanc4e.
  $galaxy = tripal_get_galaxy_connection($submission->galaxy_id);
  if ($galaxy) {

    // Create our own history for this test.
     $histories = new GalaxyHistories($galaxy);
     $ourHistory = $histories->create(array(
       'name' => "TG-" . $user->uid . "-" . $submission->galaxy_workflow_id . "-" . $submission->sid,
     ));
     $history_id = $ourHistory['id'];


    // Invoke the workflow and check for errors
    $gworkflows = new GalaxyWorkflows($galaxy);
    $invocation = $gworkflows->invoke(array(
      'workflow_id' => $submission->workflow_id,
      'parameters' => $parameters,
      'input_dataset_ids' => array(),
      'hist_id' => $history_id,
    ));

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
  else {
    throw new Exception('Connot connect to the remote galaxy server to launch the workflow');
  }
}