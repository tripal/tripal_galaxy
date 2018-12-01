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
function tripal_galaxy_get_connection($galaxy_id) {
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
  
  $connect = tripal_galaxy_split_url($galaxy_server->url);
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
 * @param $url
 *   The URL for the remote galaxy instance.
 *
 * @return
 *   An array with three keys: host, port and use_https.
 */
function tripal_galaxy_split_url($url) {

  // TODO: should this go into blend4php?

  // First check a URL with a port
  $matches = [];
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
  else {
    if (preg_match('/^(.*)\:\/\/(.+?)\/*$/', $url, $matches)) {
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
    else {
      if (preg_match('/(.*)\:(\d+)\/*$/', $url, $matches)) {
        $use_https = FALSE;
        $host = $matches[1];
        $port = $matches[2];
      }
    }
  }
  return [
    'host' => $host,
    'port' => $port,
    'use_https' => $use_https,
  ];
}

/**
 * Adds a remote Galaxy Workflow to Tripal.
 * 
 * @param $galaxy_id
 *   An ID of the galaxy server.
 * @param $values
 *   An associative array used to specify the workflow. The workflow can
 *   be identified using the Galaxy workflow ID provided using the
 *   'workflow_id' key or using the workflow name provided using the 
 *   'workflow_name' key.
 * @param $create_webform
 *   If TRUE, creates a webform for the end-user to submit this workflow
 *   using the Tripal website.  It will automatically create a history
 *   for this workflow on the remote Galaxy server as well.
 *    
 * @return
 *   A workflow object or FALSE on error.  If the workflow has already
 *   been added, then the workflow object is returned and no duplicate
 *   record is added.
 */
function tripal_galaxy_add_workflow($galaxy_id, $values, $create_webform = TRUE) {
  
  $transaction = db_transaction();
  try {
    // Connect to the galaxy server.
    $galaxy = tripal_galaxy_get_connection($galaxy_id);
    
    // Get the workflow details and see if this workflow exists
    $gworkflows = new GalaxyWorkflows($galaxy);
    $workflows = $gworkflows->index();
    if (!$workflows) {
      $error = $galaxy->getError();
      if (empty($error['message'])) {
        $error['message'] = 'No workflow using the criteria provided was found in this Galaxy server.';
      }
      drupal_set_message($error['message'], 'error');
      return FALSE;
    }  
    
    // Loop through each workflow to add it to the form
    $found_workflow = NULL;
    foreach ($workflows as $workflow) {
      if (array_key_exists('workflow_name', $values) and 
          $workflow['name'] == $values['workflow_name']) {
        $found_workflow = $workflow;
      }
      if (array_key_exists('workflow_id', $values) and
          $workflow['id'] == $values['workflow_id']) {
        $found_workflow = $workflow;
      }
    }
    
    if (!$found_workflow){
      drupal_set_message('No workflow using the criteria provided was found in this Galaxy server.', 'error');
      return FALSE;
    }
    
    // Now let's check to see if this workflow exists in the database, If it 
    // exists then just return it.
    $workflow = db_select('tripal_galaxy_workflow', 'tgw')
      ->fields('tgw')
      ->condition('tgw.workflow_id', $found_workflow['id'])
      ->condition('tgw.galaxy_id', $galaxy_id)
      ->execute()
      ->fetchObject();
    if ($workflow) {
      return $workflow;
    }  
    
    // Insert the workflow.
    $data = [
      'workflow_name' => $found_workflow['name'],
      'workflow_id' => $found_workflow['id'],
      'galaxy_id' => $galaxy_id,
      'workflow_uuid' => $found_workflow['latest_workflow_uuid']
    ];
    $success = drupal_write_record('tripal_galaxy_workflow', $data);
    if ($success) {
      $workflows = tripal_galaxy_get_workflows([
        'workflow_id' => $found_workflow['id'],
        'galaxy_id' => $galaxy_id,
      ]);
      $workflow = $workflows[0];
    }
    else {
      drupal_set_message('Could not add the workflow.', 'error');
      return FALSE;
    }
    
    // If the user wants to create the webform for end-users to submit.
    if ($create_webform) {
      module_load_include('inc', 'tripal_galaxy', 'includes/tripal_galaxy.webform');      
      $nid = tripal_galaxy_build_webform($galaxy_id, $workflow->workflow_id);
      $num_updated = db_update('tripal_galaxy_workflow')
        ->fields(['nid' => $nid])
        ->condition('workflow_id', $workflow->workflow_id)
        ->condition('galaxy_id', $galaxy_id)
        ->execute();
      $workflow->nid = $nid;
    }
    
    return $workflow;
  }
  catch (Exception $e) {
    $transaction->rollback();
    tripal_report_error('tripal_galaxy', TRIPAL_ERROR,
      'Unable to import Galaxy workflow');
    return FALSE;
  }
}

/**
 * Retrieves a list of workflows matching the given criteria.
 * 
 * @param $values
 *   An associative array used to find workflows.  The following keys are
 *   supported:
 *   - galaxy_id: finds all workflows that match the given galayx_id.
 *   - id: finds the workflow with this specific workflow ID.
 *   - workflow_id: the ID of the workflow on the remote Galaxy instance.
 *   - name: finds the workflow with a given name.  Note: the workflow name is 
 *     not guranteed to be unique. 
 *   - status: finds all workflows whose status matches the value provided.
 *   Any combination of the keys can be used.
 *   
 * @return 
 *   An array of workflow objects.
 */
function tripal_galaxy_get_workflows($values = []) {
  $workflows = [];
  
  $query = db_select('tripal_galaxy_workflow', 'tgw');
  $query->fields('tgw');
  if (array_key_exists('galaxy_id', $values)) {
    $query->condition('tgw.galaxy_id', $values['galaxy_id']);
  }
  if (array_key_exists('id', $values)) {
    $query->condition('tgw.galaxy_workflow_id', $values['id']);
  }
  if (array_key_exists('name', $values)) {
    $query->condition('tgw.workflow_name', $values['name']);
  }
  if (array_key_exists('workflow_id', $values)) {
    $query->condition('tgw.workflow_id', $values['workflow_id']);
  }
  if (array_key_exists('status', $values)) {
    $query->condition('tgw.status', $values['status']);
  }
  $results = $query->execute();
  
  while($workflow = $results->fetchObject()) {
    $workflows[] = $workflow;
  }
  
  return $workflows;
}

/**
 * Creates a new workflow submission record.
 * 
 * This function creates the record for the submission.  
 * 
 * @param $workflow
 *   An Tripal Galaxy workflow object as generated by the 
 *   tripal_galaxy_get_workflow() function.
 * @param $user
 *   The Drupal User object. This is the user who owns the submission.
 */
function tripal_galaxy_create_submission($workflow, $user) {

  $transaction = db_transaction();
  
  try {
    $data = [
      'galaxy_workflow_id' => $workflow->galaxy_workflow_id,
      'errors' => '',
      'submit_date' => REQUEST_TIME,
      'status' => 'Waiting',
      'uid' => $user->uid,
    ];
  
    // Now insert the submission into the database
    $sid = db_insert('tripal_galaxy_workflow_submission')
      ->fields($data)
      ->execute();
    

    return $sid;
    
  }
  catch (Exception $e) {
    $transaction->rollback();
    tripal_report_error('tripal_galaxy', TRIPAL_ERROR,
      'Unable to create Galaxy workflow submission');
    return FALSE;
  }
}

/**
 * Retrieves a workflow submission object using the submission ID.
 * 
 * @param $sid
 *   The submission ID of the workflow.
 *   
 * @return
 *   An object containing the submisssion information.
 */
function tripal_galaxy_get_submission($sid) {
  
    $query = db_select('tripal_galaxy_workflow_submission', 'tgws');
    $query->fields('tgws', [
      'invocation_id',
      'galaxy_workflow_id',
      'sid',
      'submit_date',
      'status',
      'email',
      'uid',
    ]);
    $query->join('tripal_galaxy_workflow', 'tgw', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
    $query->fields('tgw', ['galaxy_id', 'workflow_id', 'nid', 'workflow_name']);
    // Do a left-join on the node to get the title for the webform but in
    // the event this submission did not use a webform the left join will
    // proivde a NULL value for the webform title.
    $query->leftJoin('node', 'n', '(n.nid = tgw.nid)');
    $query->addField('n', 'title', 'webform_title');
    $query->condition('tgws.sid', $sid);    
    $submission = $query->execute()->fetchObject();
    return $submission;
}

/**
 * Retrieves a list of all workflow submissions that have not completed.
 * 
 * @return
 *   An array of submission objects.
 */
function tripal_galaxy_get_active_submissions() {
  $query = db_select('tripal_galaxy_workflow_submission', 'tgws');
  $query->fields('tgws', [
    'invocation_id',
    'galaxy_workflow_id',
    'sid',
    'submit_date',
    'status',
    'email',
    'uid',
  ]);
  $query->join('tripal_galaxy_workflow', 'tgw', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
  $query->fields('tgw', ['galaxy_id', 'workflow_id', 'nid']);
  $query->leftJoin('node', 'n', '(n.nid = tgw.nid)');
  $query->addField('n', 'title', 'webform_title');  
  $query->condition('tgws.status', ['Error', 'Completed'], 'NOT IN');
  $results = $query->execute();
  
  $submissions = [];
  while($submission = $results->fetchObject()) {
    $submissions[] = $submission;
  }
  return $submissions;
}

/**
 * Checks and updates the status of a Galaxy workflow.
 *
 * @param $sid
 *   The submission ID of the workflow.
 * @param $force .
 *   If a workflow submission is already completed this function will
 *   quickly return and not check the status again.  Setting the $force
 *   argument to TRUE will force the function to check the status.
 */
function tripal_galaxy_check_submission_status($sid = false, $force = FALSE) {

  $submissions = [];
  if (!$sid) {
    $submissions = tripal_galaxy_get_active_submissions();
  }
  elseif ($sid && !is_numeric($sid)) {
    throw new Exception('tripal_galaxy_check_submission_status(): The $sid argument is not numeric');
  }
  elseif($sid) {
    $submissions[] = tripal_galaxy_get_submission($sid);
  }

  foreach ($submissions as $submission) {
    // If the job hasn't yet been invoked then skip it.
    if (!$submission->invocation_id) {
      continue;
    }
    // If the job is complete skip it.
    if ($submission->status == 'Completed') {
      continue;
    }

    // Connect to the Galaxy instance.
    $galaxy = tripal_galaxy_get_connection($submission->galaxy_id);
    if (!$galaxy) {
      $error = $galaxy->getError();
      drupal_set_message('Could not connect to Galaxy server. ' . $error['message'], 'error');
      return false;
    }
    // Get the invocation specified.
    $gworkflows = new GalaxyWorkflows($galaxy);
    $invocation = $gworkflows->showInvocations([
      'workflow_id' => $submission->workflow_id,
      'invocation_id' => $submission->invocation_id,
    ]);
    if (!$invocation) {
      throw new Exception($galaxy->getErrorMessage());
    }
    $end_time = null;
    $update_time = date_create_from_format('Y-m-d*G:i:s.ue', $invocation['update_time'] . 'UTC');

    // Find the History for this submission.
    $history_name = tripal_galaxy_get_history_name($submission);
    $ghistories = new GalaxyHistories($galaxy);
    $histories = $ghistories->index();
    $history = '';
    if ($histories) {
      foreach ($histories as $index => $temp) {
        if ($temp['name'] == $history_name) {
          $history = $temp;
        }
      }
    }

    // check state details for all jobs
    // below are valid state names:
    //    paused
    //    ok
    //    failed_metadata
    //    upload
    //    discarded
    //    running
    //    setting_metadata
    //    error
    //    queued
    //    empty
    // if 'ok' state has value larger than 0 and all other states' values being 0,
    // then this history has completed successfully. We can set the $status = 'Completed'
    $status = '';
    if ($history) {
      $history_info = $ghistories->show(['history_id' => $history['id']]);
      $state_details = array_filter($history_info['state_details']);
      $status = json_encode($state_details);
      // if no jobs are in the state of 'paused', 'running', or 'queued', the history is completed
      if (!isset($state_details['paused']) and !isset($state_details['running']) and !isset($state_details['queued'])) {
        $ghistory_contents = new GalaxyHistoryContents($galaxy);
        $history_contents = $ghistory_contents->index(['history_id' => $history['id']]);

        // Get more details about each history content item.
        foreach ($history_contents as $index => $history_content) {
          $history_contents[$index] = $ghistory_contents->show([
            'id' => $history_content['id'],
            'history_id' => $history['id'],
          ]);
          switch ($history_content['type']) {
            case 'file':
              $params = [];
              $params['history_id'] = $history['id'];
              $params['url_only'] = true;
              $params['history_content_id'] = $history_content['id'];
              $link = $ghistory_contents->download_history_content($params);
              $history_contents[$index]['content_link'] = $link;
              break;
            default:
              break;
          }
        }

        $invocation_info['history'] = $history;
        $invocation_info['history_contents'] = $history_contents;
        $invocation_info['history_info'] = $history_info;
        if (isset($state_details['error'])) {
          $status = 'Error';
        }
        else {
          $status = 'Completed';
        }
      }
    }

    // Now inform the user that the job is done!
    $end_time = $update_time->getTimestamp();
    if (!$force && $submission->email != 'SENT') {
      tripal_galaxy_send_submission_ended_mail($submission->sid, $node->uid);
    }

    $fields = [
      'status' => $status,
      'errors' => serialize($invocation_info),
    ];
    if ($end_time != null) {
      $fields['end_time'] = $end_time;
    }
    db_update('tripal_galaxy_workflow_submission')
      ->fields($fields)
      ->condition('sid', $submission->sid)
      ->execute();
  }

  return TRUE;
}


/**
 * Constructs the history name for a given submission.
 *
 * @param $submission
 *    A submission object that contains the galaxy_workflow_id, sid,
 *    and submit_date properties.
 *
 * @return
 *   The history name.
 */
function tripal_galaxy_get_history_name($submission) {
  return "TG-" . $submission->uid . "-" . $submission->galaxy_workflow_id . "-" . $submission->sid . '-' . date('Y_m_d_H:i:s', $submission->submit_date);
}

/**
 * Invokes all submitted workflows that are in the 'Waiting' state.
 *
 * This function can be called by the tripal Job system hence the $job
 * argument. For Tripal v2 the job_id is passed, for Tripal v3 a job
 * object is passed so we'll handle both cases.
 * 
 * @param $galaxy
 *   An instance of a GalaxyInstance object.
 * @param $submission
 *   A Galaxy workflow submission object.  This object can be 
 *   retrieved using the tripal_galaxy_get_submission() function.
 * @param $parameters
 *   A mapping of tool parameters that are non-datasets parameters. The map
 *   must be in the following format:
 *   @code
 *    [
 *      {step_id_or_UUID} => [{param_name} => {value}],
 *      {step_id_or_UUID} => [{param_name} => {value}]
 *    ];
 *   @endcode
 * @param $inputs
 *   An array of file inputs.  These files should already be uploaded to the
 *   history on the Galaxy server. This array contains a mapping of workflow 
 *   inputs to datasets and dataset collections.
 *   The datasets source can be a LibraryDatasetDatasetAssociation (ldda),
 *   LibraryDataset (ld), HistoryDatasetAssociation (hda), or
 *   HistoryDatasetCollectionAssociation (hdca). The map must be in the
 *   following format
 *   @code
 *    [
 *      {step index} => [
 *        'id' => {encoded dataset ID},
 *        'src' => {'ldda'|'ld'|'hda'|'hdca'}
 *      ],
 *    ];
 *   @endcode
 *   The id's are dataset IDs and can be found using the dataset class's
 *   index() function. The dataset must be present in a history, and the
 *   dataset 'state' must be 'ok' and 'deleted' must be set to FALSE. The
 *   {step index> is the numeric value of the step in the workflow where the
 *   file is used.
 * @param $history_id
 *   The ID of the history into which the workflow should be executed.
 */
function tripal_galaxy_invoke_workflow($galaxy, $submission, $parameters, 
  $inputs, $history_id) {
  
  // Create some handy vairables for working with this submission.
  $workflow_id = $submission->workflow_id;
  $uid = $submission->uid;
  $sid = $submission->sid;
        
  // Invoke the workflow and check for errors
  $gworkflows = new GalaxyWorkflows($galaxy);
  $params = [
    'workflow_id' => $workflow_id,
    'parameters' => $parameters,
    'inputs' => $inputs,
    'history_id' => $history_id,
  ];

  $invocation = $gworkflows->invoke($params);

  if (!$invocation) {
    $error = $galaxy->getError();
    $error_msg = 'ERROR: ' . $error['message'];
    
    // If the message is an array than we'll convert it into a string.
    if (is_array($error['message'])) {
      $error_msg = 'ERROR: ';
      foreach ($error['message'] as $key => $value) {        
        $error_msg .= $key . ': ' . $value;
      }
    }
    drupal_set_message($error_msg, 'error');
    db_update('tripal_galaxy_workflow_submission')
      ->fields([
        'status' => 'Error',
        'errors' => $error_msg,
      ])
      ->condition('sid', $sid)
      ->execute();
    tripal_galaxy_send_submission_failed_mail($sid);
  }
  else {
    db_update('tripal_galaxy_workflow_submission')
      ->fields([
        'status' => 'Submitted',
        'start_time' => REQUEST_TIME,
        'end_time' => NULL,
        'invocation_id' => $invocation['id'],
      ])
      ->condition('sid', $sid)
      ->execute();
    tripal_galaxy_send_submission_start_mail($sid);
  }

}



/**
 * Uploads a file to a given history on Galaxy.
 *
 * @param $galaxy
 *   An instance of a Galaxy server object.
 * @param $fid
 *   The Drupal managed file ID.
 * @param $history_id
 *   The history ID.
 * @param $history_contents
 *   The Galaxy history contents array.
 *
 * @throws Exception
 * @return
 *   An array of the dataset details from Galaxy for the uploaded file.
 */
function tripal_galaxy_upload_file($galaxy, $fid, $history_id, $history_contents) {

  if (!$fid) {
    throw new Exception('Cannot upload a file without an fid');
  }

  $file = file_load($fid);
  $uploaded_file = NULL;


  foreach ($history_contents as $hfile) {
    if (!$hfile['deleted'] and $hfile['state'] == 'ok' and $hfile['name'] == $file->filename) {
      $uploaded_file = $hfile;
    }
  }

  // Only upload the file if it isn't already there.
  if (!$uploaded_file) {
    $file_list = [];
    $file_list[] = [
      'name' => $file->filename,
      'path' => drupal_realpath($file->uri),
    ];
    // Now upload the files.
    $report = "Uploading $file->filename...";
    if (is_object($job)) {
      $job->logMessage($report);
    }
    else {
      print $report . "\n";
    }
    $gtool = new GalaxyTools($galaxy);
    $tool = $gtool->create([
      'tool_id' => 'upload1',
      'history_id' => $history_id,
      'files' => $file_list,
    ]);
    if (!$tool) {
      $error = $galaxy->getError();
      throw new Exception($error['message']);
    }
    return $tool['outputs'][0];
  }
  else {
    $report = "File already exists in history: $file->filename...";
    if (is_object($job)) {
      $job->logMessage($report);
    }
    else {
      print $report . "\n";
    }
    return $uploaded_file;
  }
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
function tripal_galaxy_get_history(GalaxyInstance $galaxy, $history_name, &$error) {

  // TODO: should this go into blend4php?

  $ghistories = new GalaxyHistories($galaxy);

  // Look through existing histories to find what we're looking for.
  $histories = $ghistories->index();
  if (!$histories) {
    $error = $galaxy->getError();
    throw new Exception($error['message']);
  }
  foreach ($histories as $history) {
    if ($history['name'] == $history_name) {
      return $history;
    }
  }

  // If we're here then the history doesn't exist, so create one.
  $history = $ghistories->create([
    'name' => $history_name,
  ]);
  if (!$history) {
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
 * @return
 *   Returns TRUE if accessible. FALSE otherwise.  A Drupal message is
 *   also provided that indicates if the test was successful.
 */
function tripal_galaxy_test_connection($connect) {

  $library = libraries_load('blend4php');
  if (array_key_exists('error', $library)) {
    drupal_set_message($library['error message'], 'error');
    return FALSE;
  }

  if (array_key_exists('galaxy_id', $connect)) {
    $galaxy_id = $connect['galaxy_id'];
    $galaxy = tripal_galaxy_get_connection($galaxy_id);
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
    drupal_set_message(t('Successful connection to the Galaxy server (version %version)', ['%version' => $version['version_major']]));
  }
  return TRUE;
}


/**
 * Returns the URI where the Tripal Galaxy module stores files.
 *
 * This function also ensures that the path exists by creating it.
 *
 * @return
 *   A Drupal URI indicating the location where Galaxy files are housed.
 *   Returns FALSE if the location does not exist or cannot be created.
 */
function tripal_galaxy_get_files_dir() {
  global $user;

  $user_uid = md5($user->uid);

  $site_dir = 'public://tripal/files/galaxy/' . $user_uid;
  if (!file_prepare_directory($site_dir, FILE_CREATE_DIRECTORY)) {
    $message = 'Could not access the directory on the server for storing this file.';
    watchdog('tripal', $message, [], WATCHDOG_ERROR);
    return FALSE;
  }

  return $site_dir;
}

/**
 * Deletes expired histories.
 * 
 * Walks through the tripal_galaxy_workflow_submission table and deletes any 
 * workflows older than specified in the tripal_galaxy_history_age system 
 * variable.
 */
function tripal_galaxy_delete_expired_histories(){
  $max_history_age = time() - variable_get('tripal_galaxy_history_age');
  $old_workflows = db_select('tripal_galaxy_workflow_submission', 'tgws')
    ->fields('tgws')
    ->condition('start_time', $max_history_age, '<')
    ->execute();
  while ($old_workflow = $old_workflows->fetchObject()) {

    // Skip already delated workflow invocations.
    if($old_workflow->status == 'Deleted') {
      continue;
    }
    
    $tp_workflow = db_select('tripal_galaxy_workflow', 'tgw')
      ->fields('tgw')
      ->condition('galaxy_workflow_id', $old_workflow->galaxy_workflow_id, '=')
      ->execute()
      ->fetchObject();

    $node = node_load($tp_workflow->nid);
    $history_name = tripal_galaxy_get_history_name($old_workflow, $node);
    $success = tripal_galaxy_delete_remote_history($tp_workflow->galaxy_id, $history_name);
    
    if ($success) {
      drupal_set_message('Successfully deleted workflow invocation: ' . $old_workflow->invocation_id);
      //disable the workflow on the site.
      db_update('tripal_galaxy_workflow_submission')
        ->fields(array(
          'status' => 'Deleted'
        ))
        ->condition('invocation_id', $old_workflow->invocation_id)
        ->execute();
    }
    else {
      drupal_set_message('Failed to deleted workflow invocation: ' . $old_workflow->invocation_id, 'error');
    }
  }
}

/**
 * Deletes a single remote history from the remote galaxy server.
 *
 * @param $galaxy_id: A unique ID for the galaxy server. If this is provided no
 *        other arguments are needed.
 * 
 * @param $history_name
 *   The name of the history to retrieve. If the history doesn't exist then
 *   it will be created.
 * 
 */
function tripal_galaxy_delete_remote_history($galaxy_id, $history_name) {
  $error = [];
  try {
    $galaxy = tripal_galaxy_get_connection($galaxy_id);
    $history = tripal_galaxy_get_history($galaxy, $history_name, $error);
    if (!$history) {
      $error = $galaxy->getError();
      throw new Exception("Cannot find history, $history_name :" . $error['message']);
    }

    $ghistories = new GalaxyHistories($galaxy);
    $deleted = $ghistories->deleteHistory(array('history_id' => $history['id']));
    if (!$deleted) {
      $error = $galaxy->getError();
      throw new Exception("Cannot delete the history, $history_name :" . $error['message']);
    }
    return TRUE;
  }
  catch (Exception $e) {
    drupal_set_message(t('Could not delete the remote history.  Please contact the web site administrator to report this issue.'), 'error');
    watchdog_exception('tripal_galaxy', $e);
    return FALSE;
  }
}
