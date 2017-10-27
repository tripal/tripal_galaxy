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
 * @param $utl
 *   The URL for the remote galaxy instance.
 *
 * @return
 *   An array with three keys: host, port and use_https.
 */
function tripal_galaxy_split_url($url) {

  // TODO: should this go into blend4php?

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
 * @param $sid
 *   The submission ID of the workflow.
 * @param $force.
 *   If a workflow submission is already completed this function will
 *   quickly return and not check the status again.  Setting the $force
 *   argument to TRUE will force the function to check the status.
 */
function tripal_galaxy_check_submission_status($sid, $force = FALSE) {

  if (!$sid) {
    throw new Exception('tripal_galaxy_check_submission_status(): Please provide an $sid argument');

  }
  if (!is_numeric($sid)) {
    throw new Exception('tripal_galaxy_check_submission_status(): The $sid argument is not numeric');
  }

  $query = db_select('tripal_galaxy_workflow_submission', 'tgws');
  $query->fields('tgws', array('invocation_id', 'galaxy_workflow_id', 'sid', 'submit_date',  'status'));
  $query->join('tripal_galaxy_workflow', 'tgw', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
  $query->fields('tgw', array('galaxy_id', 'workflow_id', 'nid'));
  $query->condition('tgws.sid', $sid);
  $submission = $query->execute()->fetchObject();

  // If the job hasn't yet been invoked then skip it.
  if (!$submission->invocation_id) {
    return;
  }

  // If the status is already completed or error then we've already handled this
  // workflow fully and we don't need to check the full status again unless
  // the $force argument is set to TRUE.
  if (($submission->status == 'Completed' or $submission->status == 'Error') and !$force) {
    print "Workflow already handled. Status is: $submission->status\n";
    print "Skipping\n";
    return;
  }

  // Get the node for this submission.
  $node = node_load($submission->nid);

  // Connect to the Galaxy instance.
  $galaxy = tripal_galaxy_get_connection($submission->galaxy_id);

  // Get the invocation specified.
  $gworkflows = new GalaxyWorkflows($galaxy);
  $invocation = $gworkflows->showInvocations(array(
    'workflow_id' => $submission->workflow_id,
    'invocation_id' => $submission->invocation_id
  ));
  if (!$invocation) {
    throw new Exception($galaxy->getErrorMessage());
  }

  // Iterate through the invocation steps and look for any steps that
  // are in the error state.
  $state = array();
  $has_error = FALSE;
  $all_ok = FALSE;

  // If there's an altered workflow this will throw an error
  // Need to go further up to find out why this is being called and not
  // getting an invocation
  $num_steps_ok = 0;
  $num_tool_steps = 0;
  foreach ($invocation['steps'] as $step) {
    $step_index = $step['order_index'];
    $step_state = '';
    $stderr = '';
    $stdout = '';
    $misc_info = '';
    $step_name = $step['workflow_step_label'];


    // If this job has a step then get some additional information and
    // add it to our step details.
    $job = NULL;
    if ($step['job_id']) {
      $num_tool_steps++;

      // Get the STDERR and STDOUT for the job.
      $gjobs = new GalaxyJobs($galaxy);
      $job = $gjobs->show(array(
        'job_id' => $step['job_id'],
        'full' => TRUE,
      ));
      if (!$job) {
        throw new ErrorException($galaxy->getErrorMessage());
      }
      $stderr = $job['stderr'];
      $stdout = $job['stdout'];

      // Get the tool
      $gtools = new GalaxyTools($galaxy);
      $tool = $gtools->show(array('tool_id' => $job['tool_id']));
      if (!$tool) {
        throw new ErrorException($galaxy->getErrorMessage());
      }
      $step_name = $tool['name'];
      $step_state = $job['state'];
    }

    if ($step['state'] == 'error') {
      // Sometimes the error message is in a dataset. So, get the 'misc_info'
      // from that.
      if ($job and $job['outputs']['output']['id']) {
        $gdataset = new GalaxyDatasets($galaxy);
        $dataset = $gdataset->show(array('dataset_id' => $job['outputs']['output']['id']));
        if (!$dataset) {
          throw new ErrorException($galaxy->getErrorMessage());
        }
        $misc_info = $dataset['misc_info'];
      }
      $has_error = TRUE;
    }

    // Get the history contents if the job step status is 'ok'.
    $history_contents = array();
    if ($step['state'] == 'ok') {
      $num_steps_ok++;

      // Find the History for this submission.
      $history_name = tripal_galaxy_get_history_name($submission, $node);
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

      if ($history) {
        $ghistory_contents = new GalaxyHistoryContents($galaxy);
        $history_contents = $ghistory_contents->index(array('history_id' => $history['id']));
      }

      // Get more details about each history content item.
      foreach ($history_contents as $index => $history_content) {
        $history_contents[$index] = $ghistory_contents->show(array(
          'id' => $history_content['id'],
          'history_id' => $history['id'],
        ));
        switch ($history_content['type']) {
          case 'file':
            $params = array();
            $params['history_id'] = $history['id'];
            $params['url_only'] = TRUE;
            $params['history_content_id'] = $history_content['id'];
            $link = $ghistory_contents->download_history_content($params);
            $history_contents[$index]['content_link'] = $link;
            break;
          default:
            break;
        }
      } // end foreach ($history_contents as $index => $history_content) { ...
    } // end  if ($step['state'] == 'ok') { ...

    $job_id = $step['job_id'] ? $step['job_id'] : 'NULL';
    $state[$step_index][$job_id] = array(
      'step_name' => $step_name,
      'step_id' => $step['id'],
      'job_id' => $job_id,
      'state' => $step_state,
      'stderr' => $stderr,
      'stdout' => $stdout,
      'misc_info' => $misc_info,
      'history' => $history,
      'history_contents' => $history_contents,
    );

  } // end foreach ($invocation['steps'] as $step) { ...

  $status = $invocation->state;
  if($status == 'scheduled') {
    $status = 'Running';
  }
  if ($has_error) {
    print "Job Failed.\n";
    $status = 'Error';
    if (!$force) {
      tripal_galaxy_send_submission_failed_mail($sid, $node->nid);
    }
  }
  else {
    if ($num_steps_ok == $num_tool_steps) {
      print "Job Completed Successfully.\n";
      $status = 'Completed';
      if (!$force) {
        tripal_galaxy_send_submission_ended_mail($sid, $node->nid);
      }
    }
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
 * Constructs the history name for a given submission.
 *
 * @param $submission
 *    A submission object that contains the galaxy_workflow_id, sid,
 *    and submit_date properties.
 * @param $node
 *    The sumbission node object.
 *
 * @return
 *   The history name.
 */
function tripal_galaxy_get_history_name($submission, $node) {
  return "TG-" . $node->uid . "-" . $submission->galaxy_workflow_id . "-" . $submission->sid . '-' . date('Y_m_d_H:i:s', $submission->submit_date);
}
/**
 * Invokes all submitted workflows that are in the 'Waiting' state.
 *
 * This function can be called by the tripal Job system hence the $job
 * argument. For Tripal v2 the job_id is passed, for Tripal v3 a job
 * object is passed so we'll handle both cases.
 *
 * @param $sid
 *   The submission ID
 * @param $job
 *   A Tripal v3 Job object or a Tripal v2 job ID.
 */
function tripal_galaxy_invoke_workflow($sid, $job = NULL) {

  // Get the workflows that need execution
  $query = db_select('tripal_galaxy_workflow_submission', 'tgws');
  $query->fields('tgws');
  $query->join('tripal_galaxy_workflow', 'tgw', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
  $query->fields('tgw', array('galaxy_id', 'workflow_id', 'nid'));
  $query->condition('tgws.sid', $sid);
  $submission = $query->execute()->fetchObject();

  if (!$submission) {
    throw new Exception('The submission ID does not exist');
  }

  // Get a connection to the remote galaxy instance.
  $galaxy = tripal_galaxy_get_connection($submission->galaxy_id);
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
  $query->fields('wc', array('form_key', 'type', 'extra'));
  $query->condition('wsd.sid', $submission->sid);
  $query->condition('wc.nid', $submission->nid);
  $query->orderBy('wsd.cid');
  $submitted_data = $query->execute()->fetchAll();

  // Get the history that we'll use for this submission.
  $error = array();
  $history_name = tripal_galaxy_get_history_name($submission, $node);
  $history = tripal_galaxy_get_history($galaxy, $history_name, $error);
  if (!$history) {
    $error = $galaxy->getError();
    throw new Exception($error['message']);
  }

  // Get the history contents so we don't upload the same file more than once
  // in the event that this invocation occurs more than once.
  $ghistory_contents = new GalaxyHistoryContents($galaxy);
  $history_contents = $ghistory_contents->index(array('history_id' => $history['id']));
  if ($error['message']) {
    $error = $galaxy->getError();
    throw new Exception($error['message']);
  }

  // Iterate through our submitted data to look for files that need uploading.
  // For those in a dataset collection, create those collections as well.
  $collections = array();
  $inputs = array();
  foreach ($submitted_data as $data) {

    // Handle a single file upload.
    if ($data->type == 'galaxy_sfile') {
      if ($data->no == 'data_collection' or $data->no == 'existing' or preg_match('/^submitted_/', $data->no)) {
        $fid = $data->data;
        if ($fid) {
          $file = tripal_galaxy_upload_file($galaxy, $fid, $history['id'], $history_contents);
          $inputs[$data->form_key] = $file;
        }
      }
      else if ($data->no == 'site_wide'){
        $fid = $data->data;
        if ($fid) {
          $swfile = db_select('tripal_galaxy_site_files', 'tgsf')
            ->fields('tgsf')
            ->condition('fid', $fid)
            ->execute()
            ->fetchObject();
          if (!$swfile) {
            throw new Exception('Cannot find site-wide file with fid: ' . $fid);
          }
          // If this is a remote file....
          if ($swfile->url) {

          }
          // not a remote file.
          else {
            $file = tripal_galaxy_upload_file($galaxy, $fid, $history['id'], $history_contents);
            $inputs[$data->form_key] = $file;
          }
        }
      }
    }
    else if($data->type == 'galaxy_sflist') {

      if ($data->no == 'existing' or preg_match('/^submitted_/', $data->no)) {
        $fids = $data->data;
        if (!$fids) {
          continue;
        }
        $set = explode('|', $fids);

        // Create the paired dataset.
        $collections[$data->form_key] = array(
          'collection_type' => 'list',
          'instance_type' => 'history',
          'name' => $data->form_key,
          'history_id' => $history['id'],
          'element_identifiers' => array(),
        );

        for ($i = 0; $i < count($set); $i++) {
          // Upload each file.
          if ($set[$i]) {
            $dataset1 = tripal_galaxy_upload_file($galaxy, $set[$i], $history['id'], $history_contents);

            // Next add the element.
            $collections[$data->form_key]['element_identifiers'][] = array(
              'id' => $dataset1['id'],
              'name' => $dataset1['name'],
              'src' => 'hda'
            );
          }
        } // end for ($i = 0; $i < count($set1); $i++) {
      }
    }
    else if($data->type == 'galaxy_pflist') {
      if ($data->no == 'existing' or preg_match('/^submitted_/', $data->no)) {
        $fids = $data->data;
        if (!$fids) {
          continue;
        }
        $set1 = array();
        $set2 = array();
        if ($fids) {
          list($set1, $set2) = explode(',', $fids);
          $set1 = explode('|', $set1);
          $set2 = explode('|', $set2);
        }

        // Create the paired dataset.
        $collections[$data->form_key] = array(
          'collection_type' => 'list:paired',
          'instance_type' => 'history',
          'name' => $data->form_key,
          'history_id' => $history['id'],
          'element_identifiers' => array(),
        );

        for ($i = 0; $i < count($set1); $i++) {
          // Upload each file.
          if ($set1[$i] and $set2[$i]) {
            $dataset1 = tripal_galaxy_upload_file($galaxy, $set1[$i], $history['id'], $history_contents);
            $dataset2 = tripal_galaxy_upload_file($galaxy, $set2[$i], $history['id'], $history_contents);

            // Next create the paired dataset collection array.
            $collections[$data->form_key]['element_identifiers'][] = array(
              'name' => $dataset1['name'] . ' and ' . $dataset2['name'],
              'collection_type' => 'paired',
              'src' => 'new_collection',
              'element_identifiers' => array(
                array(
                  'id' => $dataset1['id'],
                  'name' => 'forward',
                  'src' => 'hda'
                ),
                array(
                  'id' => $dataset2['id'],
                  'name' => 'reverse',
                  'src' => 'hda'
                ),
              ),
            );
          }
        } // end for ($i = 0; $i < count($set1); $i++) {
      } // end if ($data->no == 'existing' or preg_match('/^submitted_/', $data->no)) {
    } // end else if($data->type == 'galaxy_pflist') {
  } // foreach ($submitted_data as $data) {

  // Now create a dataset collections
  foreach ($collections as $form_key => $ds_details) {

    // Check for the dataset in the history before trying to readd it.
    $found_collection = FALSE;
    foreach ($history_contents as $hcontent) {
      if ($hcontent['name'] == $form_key and $hcontent[deleted] == 0){
        $inputs[$form_key] = $hcontent;
        $found_collection = TRUE;
      }
    }
    if ($found_collection) {
      $report = "Collection, $form_key, already exists.";
      if (is_object($job)) {
        $job->logMessage($report);
      }
      else {
        print $report . "\n";
      }
      continue;
    }

    // Create the collection!
    $report = "Creating Collection: $form_key";
    if (is_object($job)) {
      $job->logMessage($report);
    }
    else {
      print $report . "\n";
    }
    $gds_collection = new GalaxyDatasetCollections($galaxy);
    $ds_collection = $gds_collection->create($ds_details);
    if (!$ds_collection) {
      $error = $galaxy->getError();
      throw new Exception($error['message']);
    }
    $inputs[$form_key] = $ds_collection;
  }

  // Next we need to re-get the history content to get the IDs of the new files.
  $history_contents = $ghistory_contents->index(array('history_id' => $history['id']));
  $input_datasets = array();

  // Iterate through the submitted data values and create an input aray
  // for Galaxy workflow submission.
  foreach ($submitted_data as $data) {
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
    else if ($data->type == 'galaxy_sfile') {
      $file = $inputs[$data->form_key];
      foreach ($history_contents as $hcontent) {
        if ($hcontent['name'] == $file['name']) {
          $value = $hcontent['id'];
          $input_datasets[$step_index] = array(
            'id' => $value,
            'src' => 'hda',
          );
        }
      }
    }
    else if($data->type == 'galaxy_sflist') {
      $dataset = $inputs[$data->form_key];
      $value = $dataset['id'];
      $input_datasets[$step_index] = array(
        'id' => $value,
        'src' => 'hdca',
      );
    }
    else if($data->type == 'galaxy_pflist') {
      $dataset = $inputs[$data->form_key];
      $value = $dataset['id'];
      $input_datasets[$step_index] = array(
        'id' => $value,
        'src' => 'hdca',
      );
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

  // Invoke the workflow and check for errors
  $gworkflows = new GalaxyWorkflows($galaxy);
  $params = array(
    'workflow_id' => $submission->workflow_id,
    'parameters' => $parameters,
    'inputs' => $input_datasets,
    'history_id' => $history['id'],
  );
  $invocation = $gworkflows->invoke($params);

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
    $file_list = array();
    $file_list[] = array(
      'name' => $file->filename,
      'path' =>  drupal_realpath($file->uri)
    );
    // Now upload the files.
    $report =  "Uploading $file->filename...";
    if (is_object($job)) {
      $job->logMessage($report);
    }
    else {
      print $report . "\n";
    }
    $gtool = new GalaxyTools($galaxy);
    $tool = $gtool->create(array(
      'tool_id' => 'upload1',
      'history_id' => $history_id,
      'files' => $file_list,
    ));
    if (!$tool) {
      $error = $galaxy->getError();
      throw new Exception($error['message']);
    }
    return $tool['outputs'][0];
  }
  else {
    $report =  "File already exists in history: $file->filename...";
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
    drupal_set_message(t('Successful connection to the Galaxy server (version %version)', array('%version' => $version['version_major'])));
  }
  return TRUE;
}

/**
 * API call to edit an existing user's custom quota
 *
 *
 * @return Associative array of user's new quota and exp_date from db table.
 **/
 function tripal_galaxy_admin_edit_user_quota($uid, $new_quota, $new_exp_date) {

 }



/**
 * Sets a user's file space quota and default file expiration.
 *
 * @param $uid
 *   The User ID for whom the quota will be set.
 * @param $quota
 *   The quota
 * @param $expriation
 *
 * @return
 *   The inserted record.
 */
 function tripal_galaxy_set_user_quota($uid, $quota, $expriation) {

   $values = array(
     'uid' => $uid,
     'custom_quota' => $quota,
     'custom_expiration' => $expriation,
   );
   return db_insert('tripal_galaxy_custom_quota')
     ->fields($values)
     ->execute();
 }

 /**
  * Retrieves the user's quote and default days to expire.
  *
  * @param $uid
  *   The User ID.
  *
  * @return
  *   An associative array containing the quota and default days to
  *   expire.
  */
 function tripal_galaxy_get_user_quota($uid) {
   $quota =  db_select('tripal_galaxy_custom_quota', 'tgcq')
     ->fields('tgcq', array('custom_quota', 'custom_expiration'))
     ->condition('uid', $uid)
     ->execute()
     ->fetchObject();
   if (!$quota) {
     $quota = new stdClass();
     $quota->custom_quota = variable_get('tripal_galaxy_default_file_quota', pow(20,6));
     $quota->custom_expiration = variable_get('tripal_galaxy_default_file_expiration', '60');
   }
   return $quota;
}

/**
 * Retrieves the current size of all files uploaded by the user.
 *
 * @param $uid
 *   The User ID.
 *
 * @return
 *   The total number of bytes currently used.
 */
function tripal_galaxy_get_user_usage($uid) {
  // Get the user's current file usage
  $sql = "
    SELECT DISTINCT FU.fid
    FROM {file_usage} FU
      INNER JOIN {file_managed} FM ON FM.fid = FU.fid and FU.module = 'tripal_galaxy'
    WHERE FM.uid = :uid
  ";
  $fids = db_query($sql, array(':uid' => $uid));
  $total_size = 0;
  while($fid = $fids->fetchObject()) {
    $sql = "SELECT filesize FROM {file_managed} WHERE fid = :fid";
    $total_size += db_query($sql, array (':fid' => $fid->fid))->fetchObject()->filesize;
  }
  return $total_size;
}
 /**
  * Removes a user's file space and default file expiration.
  *
  * @param $uid
  *   The User ID for whom the quota will be removed.
  *
  * @return
  */
 function tripal_galaxy_remove_user_quota($uid) {
   db_delete('tripal_galaxy_custom_quota')
     ->condition('uid', $uid)
     ->execute();
 }


 /**
  * Formats a size (in bytes) in human readable format.
  *
  * Function taken from php.net
  *
  * @param $bytes
  *   The size of the file in bytes
  * @param $precision
  *   The number of decimal places to use in the final number if needed
  *
  * @return string
  *   A formatted string indicating the size of the file
  */
 function tripal_galaxy_format_bytes($bytes, $precision = 2) {
   $units = array('B', 'KB', 'MB', 'GB', 'TB');

   $bytes = max($bytes, 0);
   $pow = floor(($bytes ? log($bytes) : 0) / log(1024));
   $pow = min($pow, count($units) - 1);

   // Uncomment one of the following alternatives
   $bytes /= pow(1000, $pow);
   // $bytes /= (1 << (10 * $pow));

   return round($bytes, $precision) . '' . $units[$pow];
 }

/**
 * Checks if a file needs to be expired.
 *
**/
function tripal_galaxy_check_file_expiration() {
  $results = db_select('tripal_galaxy_expiration_files', 'tgfe')
    ->fields('tgfe')
    ->execute();
  while($result = $results->fetchObject()) {
    if(time() <= $result->expiration_date){
      // remove the file from the server as it is outdated and has not been
      // renewed through user action
      // TRUE to force the file to be deleted and dropped from all tables
      file_delete(file_load($result->fid), TRUE);
    }
  }
}

/**
 * Returns the IRI where the Tripal Galaxy module stores files.
 *
 * This function also ensures that the path exists by creating it.
 *
 * @return
 *   A Drupal IRI indicating the location where Galaxy files are housed.
 *   Returns FALSE if the location does not exist or cannot be created.
 */
function tripal_galaxy_get_files_dir() {
  global $user;

  $user_uuid = md5($user->uuid);

  $site_dir = 'public://tripal/files/galaxy/' . $user_uuid;
  if (!file_prepare_directory($site_dir, FILE_CREATE_DIRECTORY)) {
    $message = 'Could not access the directory on the server for storing this file.';
    watchdog('tripal', $message, array(), WATCHDOG_ERROR);
    return FALSE;
  }

  return $site_dir;
}