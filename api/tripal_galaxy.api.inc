<?php

/**
 * @file
 * Provides an application programming interface (API) for the Tripal Galaxy
 * module.
 */

/**
 * @defgroup tripal_galaxy_api Tripal Galaxy API
 * @{
 * Provides an application programming interface (API) for integrating Tripal
 * and Galaxy. It uses the blend4php library to communicate with a remote Galaxy
 * instances but ensures that other developers who want to integrate with the 
 * features of the Tripal Galaxy module cab do so.
 * @}
 */

/**
 * Retreives a GalaxyInstance objects using a galaxy_id
 *
 * @param int $galaxy_id
 *   The ID of a galaxy server.
 *
 * @return GalaxyInstance
 *   A galaxyInstance object or FALSE on error.
 *
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_get_connection(int $galaxy_id) {
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
 * @param string $url
 *   The URL for the remote galaxy instance.
 *
 * @return array
 *   An array with three keys: host, port and use_https.
 */
function tripal_galaxy_split_url(string $url) {

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
 * @param int $galaxy_id
 *   An ID of the galaxy server.
 * @param array $values
 *   An associative array used to specify the workflow. The workflow can
 *   be identified using the Galaxy workflow ID provided using the
 *   'workflow_id' key or using the workflow name provided using the 
 *   'workflow_name' key.
 * @param bool $create_webform
 *   If TRUE, creates a webform for the end-user to submit this workflow
 *   using the Tripal website.  It will automatically create a history
 *   for this workflow on the remote Galaxy server as well.
 *    
 * @return object
 *   A workflow object or FALSE on error.  If the workflow has already
 *   been added, then the workflow object is returned and no duplicate
 *   record is added.
 *   
 *  @ingroup tripal_galaxy_api
 */
function tripal_galaxy_add_workflow(int $galaxy_id, array $values, bool $create_webform = TRUE) {
  
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
 * @param array $values
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
 * @return array
 *   An array of workflow objects.
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_get_workflows(array $values = []) {
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
 * Initializes a new workflow submission record.
 * 
 * This function creates the record for the submission.  
 * 
 * @param object $workflow
 *   An workflow object as generated by the  tripal_galaxy_get_workflow() 
 *   function.
 * @param object $user
 *   The Drupal User object. This is the user who owns the submission.
 *   
 * @return int|bool
 *   The submission ID on success, FALSE on failure.
 *     
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_init_submission(object $workflow, object $user) {

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
 * @param int $sid
 *   The submission ID of the workflow.
 *   
 * @return object
 *   An object containing the submisssion information.
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_get_submission(int $sid) {
  
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
 * Retrieves a list of Galaxy servers that are known to Tripal.
 * 
 * @return array
 *   An associative array of galaxy server objects.  Note, the returned 
 *   objects are different from the GalaxyInstance objects provided by 
 *   blend4php.  These objects house the information that Tripal maintains
 *   about each of these servers.
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_get_galaxies() {
  
  $galaxies = [];
  $results = db_select('tripal_galaxy', 'tg')
    ->fields('tg')
    ->execute();
  
  while ($galaxy = $results->fetchObject()) {
    $galaxies[] = $galaxy;
  }
  return $galaxies;
}

/**
 * Retrieves a Galaxy object.
 * 
 * @param int $galaxy_id
 *   The ID of the galaxy server to retrieve.  A list of all galxy servers
 *   known to Tripal can be retrieved using the tripal_galaxy_get_galaxies()
 *   function.
 *
 * @return object
 *   A galaxy object  Note, the returned
 *   object is different from the GalaxyInstance object provided by
 *   blend4php.  This objects house the information that Tripal maintains
 *   about each the servers.
 * 
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_get_galaxy(int $galaxy_id) {
  
  $galaxies = [];
  $galaxy = db_select('tripal_galaxy', 'tg')
    ->fields('tg')
    ->condition('galaxy_id', $galaxy_id)
    ->execute()
    ->fetchObject();
  
  return $galaxy;
}

/**
 * Adds a new Galaxy server to Tripal.
 * 
 * @param array $values
 *   An associative array containing the following key/value pairs:
 *   - servername:  A human-readable name for the Galaxy server.  The servername
 *     must be unique and not already present.
 *   - description:  (optional) a description about this server.
 *   - url: The full URL for the server (e.g. https://usegalaxy.org/)
 *   - username: The name of the user to connect as.
 *   - api_key: The API key of the user.  This allows Tripal to access the 
 *     Galaxy server's RESTful API services.
 *   - uid: The Drupal User ID who created this record.
 *   
 * @return object
 *   The newly created Galaxy object or FALSE on error.  If the record already
 *   exists the object is returned and no duplicated record is added.
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_add_galaxy(array $values) {

  $required_args = ['servername', 'url', 'username', 'api_key', 'uid'];
  foreach ($required_args as $key) {
    if (!array_key_exists($key, $values)) {
      tripal_report_error('tripal_galaxy', TRIPAL_ERROR,
        'tripal_galaxy_add_galaxy: missing required field "' . $key . '"');
      return FALSE;
    }
  }
  
  // The servername can't already exist. 
  $exists = db_select('tripal_galaxy', 'tg')
     ->fields('tg')
     ->condition('servername', $values['servername'])
     ->execute()
     ->fetchObject();
   if ($exists) {
     return $exists;
   }
  
  
  // Create the element.
  $data = array(
    'servername' => $values['servername'],
    'description' => $values['description'],
    'url' => $values['url'],
    'username' => $values['username'],
    'api_key' => $values['api_key'],
    'uid' => $values['uid'],
  );
  drupal_write_record('tripal_galaxy', $data);
  
  return tripal_galaxy_get_galaxy($data['galaxy_id']);
}

/**
 * Retrieves a list of all workflow submissions that have not completed.
 * 
 * @return array
 *   An array of submission objects.
 *   
 *  @ingroup tripal_galaxy_api
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
 * @param int $sid
 *   The submission ID of the workflow.
 * @param bool $force
 *   If a workflow submission is already completed this function will
 *   quickly return and not check the status again.  Setting the $force
 *   argument to TRUE will force the function to check the status.
 *   
 * @return bool
 *   Returns TRUE on successful checking of the status, FALSE if a problem
 *   occured.
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_check_submission_status(int $sid = NULL, bool $force = FALSE) {

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
 * @param object $submission
 *    A submission object that contains the galaxy_workflow_id, sid,
 *    and submit_date properties.
 *
 * @return string
 *   The history name.
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_get_history_name(object $submission) {
  return "TG-" . $submission->uid . "-" . $submission->galaxy_workflow_id . "-" . $submission->sid . '-' . date('Y_m_d_H:i:s', $submission->submit_date);
}

/**
 * Retrieves an array of settings used by the workflow.
 * 
 * This is a helpful function to help a developer understand how the $parameters
 * array should be structured when passing into the 
 * tripal_galaxy_invoke_workflow() function.
 * 
 * @param GalaxyInstance $galaxy
 *   A GalaxyInstance object.
 * @param int $workflow_id
 *   The ID of the workflow to retrieve settings for.  This is the Galaxy ID 
 *   for the workflow.
 *   
 * @return
 *   An array compatible with the $parameters argument for the 
 *   tripal_galaxy_invoke_wokflow(). Values are populatd with appropriate
 *   defaults.  Returns  FALSE if the workflow settings could not be retrieved.
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_get_workflow_defaults(GalaxyInstance $galaxy, int $workflow_id) {
  
  $gworkflows = new GalaxyWorkflows($galaxy);
  $workflow = $gworkflows->show(['workflow_id' => $workflow_id]);
  
  if (!$workflow) {
    $error = $galaxy->getError();
    if (empty($error['message'])) {
      $error['message'] = 'No workflow is found in this Galaxy server for the user you used
        to connect to the Galaxy server.';
    }
    drupal_set_message($error['message'], 'error');
    return FALSE;
  }
  
  $parameters = [];
  foreach ($workflow['steps'] as $step => $details) {
    $step_inputs = $details['input_steps'];
    $step_input_key = '';
    _tripal_galaxy_get_workflow_defaults($details['tool_inputs'], $parameters[$step], $step_inputs, $step_input_key);
  }
 
  return $parameters;
}

/**
 * A recursive helper function for tripal_galaxy_get_workflow_defaults.
 * 
 * @param array $tool_inputs
 *   The 'tool_inputs' array for the step, or the sub elements on a recursive 
 *   call.
 * @param array $parameters
 *   A pass-by-reference of the $parameters array. 
 * @param array $step_inputs
 *   The 'input_steps' array for the step.  This is always passed as is at
 *   every recursion level.
 * @param string $step_input_key
 *   The key used to lookup up if an element has a key in the $step_inputs. For
 *   nested keys they are concatenated. with a '|' separating each level.
 */
function _tripal_galaxy_get_workflow_defaults(array $tool_inputs, array &$parameters, 
    array$step_inputs, string $step_input_key) {
  
  foreach ($tool_inputs as $key => $value) {
    
    // Skip keys that are just internal to Galaxy.
    if (preg_match('/^__/', $key)) {
      continue;
    }
    
    $this_step_input_key = $step_input_key . $key;
    
    
    // If this is an array then we want to recurse.
    if (is_array($value)) {
      $parameters[$key] = [];
      _tripal_galaxy_get_workflow_defaults($value, $parameters[$key], $step_inputs, $this_step_input_key . '|');
      if (empty($parameters[$key])) {
        if (is_array($step_inputs) and array_key_exists($this_step_input_key, $step_inputs)) {
          $parameters[$key] = $step_inputs[$this_step_input_key];
        }
        else {
          $parameters[$key] = '';
        }
      }
    }
    // Otherwise just set the value.
    else {
      // If this key is an input from another tool we need to get that
      // information.
      if (is_array($step_inputs) and array_key_exists($this_step_input_key, $step_inputs)) {
        $parameters[$key] = $step_inputs[$this_step_input_key];
      }
      else {
        $parameters[$key] = $value;
      }
    }
  }
}

/**
 * Invokes all submitted workflows that are in the 'Waiting' state.
 *
 * This function can be called by the tripal Job system hence the $job
 * argument. For Tripal v2 the job_id is passed, for Tripal v3 a job
 * object is passed so we'll handle both cases.
 * 
 * @param GalxyInstnace $galaxy
 *   An instance of a GalaxyInstance object.
 * @param object $submission
 *   A Galaxy workflow submission object.  This object can be 
 *   retrieved using the tripal_galaxy_get_submission() function.
 * @param array $parameters
 *   A mapping of tool parameters that are non-datasets parameters. The map
 *   must be in the following format:
 *   @code
 *    [
 *      {step_id_or_UUID} => [{param_name} => {value}],
 *      {step_id_or_UUID} => [{param_name} => {value}]
 *    ];
 *   @endcode
 * @param array $inputs
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
 *   The id's are data set IDs and can be found using the data set class's
 *   index() function. The data set must be present in a history, and the
 *   data set 'state' must be 'ok' and 'deleted' must be set to FALSE. The
 *   {step index> is the numeric value of the step in the workflow where the
 *   file is used.
 * @param array $history
 *   A history record as returned by the function tripal_galaxy_get_history().
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_invoke_workflow(GalaxyInstance $galaxy, object $submission, 
  array $parameters, array $inputs, array $history) {
  
  $history_id = $history['id'];
  
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
 * @param object $galaxy
 *   An instance of a Galaxy server object.
 * @param int $fid
 *   The Drupal managed file ID.
 * @param int $history_id
 *   The history ID.
 * @param array $history_contents
 *   The Galaxy history contents array as returned by the 
 *   GalaxyHistoryContents::index function.  
 *
 * @throws Exception
 * @return array
 *   An array of the dataset details from Galaxy for the uploaded file.
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_upload_file(object $galaxy, int $fid, int $history_id, array $history_contents) {

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
 * If the history does not exist then one is created.
 *
 * @param object $galaxy
 *   A GalaxyInstance object
 * @param string $history_name
 *   The name of the history to retrieve. If the history doesn't exist then
 *   it will be created.
 * @param array $error
 *   An empty array into which the error type and message will be placed
 *   if an error occurs.
 *
 * @return array|bool
 *   A history array for the specified history.  If a failure occured then
 *   FALSE is returned and the $error argument is set.
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_get_history(GalaxyInstance $galaxy, string $history_name, array &$error) {

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
 * @param array $connect
 *   An array of the following:
 *    - galaxy_id: A unique ID for the galaxy server. If this is provided no
 *        other arguments are needed.
 *    - host: The DNS hostname of the galaxy server.
 *    - port: The TCP port for the server.
 *    - use_https:  Set to TRUE of the server uses HTTPS
 *
 * @return bool
 *   Returns TRUE if accessible. FALSE otherwise.  A Drupal message is
 *   also provided that indicates if the test was successful.
 *   
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_test_connection(array $connect) {

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
 * @return string|bool
 *   A Drupal URI indicating the location where Galaxy files are housed.
 *   Returns FALSE if the location does not exist or cannot be created.
 *   
 * @ingroup tripal_galaxy_api
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
 * 
 * @ingroup tripal_galaxy_api
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
 * @param int $galaxy_id: 
 *   A unique ID for the galaxy server. If this is provided no other 
 *   arguments are needed.
 * 
 * @param string $history_name
 *   The name of the history to retrieve. If the history doesn't exist then
 *   it will be created.
 *   
 * @return bool
 *   TRUE if the deletion was successful, FALSE otherwise.
 * @ingroup tripal_galaxy_api 
 */
function tripal_galaxy_delete_remote_history(int $galaxy_id, string $history_name) {
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
