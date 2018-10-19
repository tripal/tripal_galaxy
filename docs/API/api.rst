Tripal Galaxy API
==============================

.. note::

  Use of these API functions is predicated on the installation of the Tripal Galaxy module and the blend4php library.

The Tripal Galaxy API allows you to plugin to the integration between Tripal and Galaxy and customize some or all of the process.  By default the Tripal Galaxy module provides an interface that allows a site to offer a step-by-step interface for a workflow, maintaining the look-and-feel of the site.  If that functionality is sufficient you will not need this API documentation.


Open a Connection
-----------------
Before any communication between Tripal and Galaxy can happen a connection between the two must be made. The tripal_galaxy_get_connection($galaxy_id) retreives a GalaxyInstance objects using a galaxy_id thereby opening the required communication path.

.. code-block:: php

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

Here is an example of this API function in use:

.. code-block:: php

  // Connect to the Galaxy instance.
  $galaxy = tripal_galaxy_get_connection($submission->galaxy_id);
  if (!$galaxy) {
    $error = $galaxy->getError();
    drupal_set_message('Could not connect to Galaxy server. ' . $error['message'], 'error');
    return false;
  }

The returned object, $galaxy from the example above, can then be used to tap into the GalaxyWorflows class from the blend4php library, like in this example where a workflow is invoked:
  
.. code-block:: php

  // Get the invocation specified.
  $gworkflows = new GalaxyWorkflows($galaxy);
  $invocation = $gworkflows->showInvocations([
    'workflow_id' => $submission->workflow_id,
    'invocation_id' => $submission->invocation_id,
  ]);
  if (!$invocation) {
    throw new Exception($galaxy->getErrorMessage());
  }


Split a URL
-----------
To successfully connect to a Galaxy instance the url must be split into it's parts: host, port, and protocol.

.. code-block:: php

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

Here is an example of this in use:

.. code-block:: php

  $connect = tripal_galaxy_split_url($galaxy_server->url);
  $galaxy = new GalaxyInstance($connect['host'], $connect['port'], $connect['use_https']);
  $galaxy->setAPIKey($galaxy_server->api_key);
  $error = $galaxy->getErrorType();
  if ($error) {
    return FALSE;
  }


Check the status of a workflow submission
-----------------------------------------
Communication between Galaxy and Tripal needs to be initiated and specific information needs to be requested. Given that, this function checks the status of a Galaxy workflow and updates the status tripal_galaxy_workflow_submission table with the results. A workflow on the Tripal Galaxy side will have one of 4 statuses: Waiting, Submitted, Completed or Error. 

.. code-block:: php

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
  function tripal_galaxy_check_submission_status($sid, $force = FALSE) {

    if (!$sid) {
      throw new Exception('tripal_galaxy_check_submission_status(): Please provide an $sid argument');

    }
    if (!is_numeric($sid)) {
      throw new Exception('tripal_galaxy_check_submission_status(): The $sid argument is not numeric');
    }

    $query = db_select('tripal_galaxy_workflow_submission', 'tgws');
    $query->fields('tgws', [
      'invocation_id',
      'galaxy_workflow_id',
      'sid',
      'submit_date',
      'status',
    ]);
    $query->join('tripal_galaxy_workflow', 'tgw', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
    $query->fields('tgw', ['galaxy_id', 'workflow_id', 'nid']);
    $query->condition('tgws.sid', $sid);
    $submission = $query->execute()->fetchObject();

    // If the job hasn't yet been invoked then skip it.
    if (!$submission->invocation_id) {
      return;
    }

    // Get the node for this submission.
    $node = node_load($submission->nid);

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
    $end_time = NULL;
    $update_time = date_create_from_format('Y-m-d*G:i:s.ue', $invocation['update_time'] . 'UTC');

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
              $params['url_only'] = TRUE;
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
        $status = 'Completed';
      }
    }

    // Now inform the user that the job is done!
    $end_time = $update_time->getTimestamp();
    if (!$force) {
      tripal_galaxy_send_submission_ended_mail($sid, $node->nid);
    }

    $fields = [
      'status' => $status,
      'errors' => serialize($invocation_info),
    ];
    if ($end_time != NULL) {
      $fields['end_time'] = $end_time;
    }
    db_update('tripal_galaxy_workflow_submission')
      ->fields($fields)
      ->condition('sid', $sid)
      ->execute();
    return TRUE;
  }

Here is an example of this in use:

.. code-block:: php

  // Update the status of running workflows
  $query = db_select('tripal_galaxy_workflow_submission', 'tgws');
  $query->fields('tgws', ['sid']);
  $query->condition('tgws.status', ['Error', 'Completed'], 'NOT IN');
  $submissions = $query->execute();
  foreach ($submissions as $submission) {
    tripal_galaxy_check_submission_status($submission->sid);
  }


Get a History Name
------------------
In Galaxy a History is the data and analysis results of a workflow. For more information on what histories are in Galaxy you can check out their tutorial page: https://galaxyproject.org/tutorials/histories/.

When Tripal Galaxy creates a workflow within Galaxy it structures the History name as: "TG-NodeId-GalaxyWorflowID-SubmissionID-DateTimeOfSubmission". 

.. code-block:: php

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

Here is an example of how to use it:

.. code-block:: php

  // Retrieve the $submission object from the tripal_galaxy_workflow_table
  $query = db_select('tripal_galaxy_workflow_submission', 'tgws');
  $query->fields('tgws', [
    'sid',
    'galaxy_workflow_id',
    'status',
    'errors',
    'submit_date',
    'start_time',
    'end_time',
    'invocation_id',
  ]);
  $query->join('tripal_galaxy_workflow', 'tgw', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
  $query->fields('tgw', ['nid', 'galaxy_id', 'workflow_id']);
  $query->condition('tgws.sid', $sid);
  $submission = $query->execute()->fetchObject();

  // Retrieve the $node oject
  $node = node_load($submission->nid);

  // Now call the API function to get the history_name.
  $history_name = tripal_galaxy_get_history_name($old_workflow, $node);


Invoke a Workflow
-----------------
Workflows are the multistep process through which data is submitted, analysed, processed, and then results provided. Workflows are created in Galaxy, they cannot be created in Tripal Galaxy. One workflow can and probably will have many different histories, a history is the data and results from a workflow.

For more information on creating and editing workflows please see: https://galaxyproject.org/tutorials/g101/#creating-and-editing-a-workflow

Before invoking the workflow all data files need to be uploaded to Galaxy and their unique "History Content API ID".

Invoking a workflow using this api function requires a complex array that is mapped directly to the workflow that is being invoked. Tripal Galaxy uses the data returned from the webform submission to build this array, for an example of this you can review tripal_galaxy_invoke_webform_submission in tripal_galaxy.webform.inc. 

Here is an example of an array that would be passed in as the $parameters argument:

.. code-block:: php

  Array
  (
      [0] => Array
          (
              [Data File] => 70eec96181a992f8
          )

      [1] => Array
          (
              [Data File] => 8317ee2b0d0f62d9
          )

      [2] => Array
          (
              [expression_data] => Array
                  (
                      [step_output] => output
                      [source_step] => 0
                  )

              [echo] => no
          )

      [3] => Array
          (
              [preprocessing_workspace] => Array
                  (
                      [step_output] => preprocessing_workspace
                      [source_step] => 2
                  )

              [height_cut] => 15
              [trait_data] => Array
                  (
                      [step_output] => output
                      [source_step] => 1
                  )

              [echo] => no
          )

  )

This array is what is passed to Galaxy for this workflow:

.. image:: ./galaxy_workflow_canvas.png

The array elements map to steps in the workflow and the required information in that step.

.. code-block:: php

  /**
  * Invokes all submitted workflows that are in the 'Waiting' state.
  *
  * This function can be called by the tripal Job system hence the $job
  * argument. For Tripal v2 the job_id is passed, for Tripal v3 a job
  * object is passed so we'll handle both cases.
  * 
  * @param $galaxy
  *   An instance of a GalaxyInstance object.
  * @param $workflow_id
  *   The workflow ID on the remote galaxy server for the workflow that is to
  *   be submitted.
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
  * @param $sid
  *    The unique identified from the tripal_galaxy_workflow_submission table.
  */
  function tripal_galaxy_invoke_workflow($galaxy, $workflow_id, $parameters, 
    $inputs, $history_id, $sid) {
    print_r($parameters);
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
      drupal_set_message($error['message'], 'error');
      db_update('tripal_galaxy_workflow_submission')
        ->fields([
          'status' => 'Error',
          'errors' => $error['message'],
        ])
        ->condition('sid', $sid)
        ->execute();
        tripal_galaxy_send_submission_failed_mail($submission->sid, $node->uid);
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
        tripal_galaxy_send_submission_start_mail($submission->sid, $node->uid);
    }

  }
  //TODO FROM HERE

Upload a file to Galaxy
-----------------------

For loading files from your local Tripal site into Galaxy.

.. code-block:: php

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

For a an example of this function in use see the tripal_galaxy_invoke_webform_submission() function in tripal_galaxy.webform.inc. 

Retrieving a history from Galaxy
--------------------------------

The history is the results from the invocation of the workflow. Tripal Galaxy builds history names in a specific format so that histories are easily accessible and renderable within the Tripal Galaxy interface.

.. code-block:: php

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

This function is used frequently throughout the Tripal Galaxy module, here is an example of its use:

.. code-block:: php  

  // Get the history that we'll use for this submission.
  $error = [];
  $history_name = tripal_galaxy_get_history_name($submission, $node);
  $history = tripal_galaxy_get_history($galaxy, $history_name, $error);
  if (!$history) {
    $error = $galaxy->getError();
    throw new Exception($error['message']);
  }


Test if a Galaxy server is accessible.
--------------------------------------
Workflows are hosted and invoked on the external Galaxy servers so if a Galaxy server is not accessible no actions can be taken on the workflow including submissions, status updates, or results display. 

.. code-block:: php

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
      drupal_set_message(t('Successful connection to the Galaxy server (version %version)', ['%version' => $version['version_major']]));
    }
    return TRUE;
  }


Tripal Galaxy file storage locator
----------------------------------
Tripal Galaxy and Tripal store user files in different locations, this function returns the location of Tripal Galaxy user files. 

.. code-block:: php

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

Here is an exmaple of use within the tripal_galaxy.adin_files.inc file, lines 234-253:

.. code-block:: php

  if ($file_upload) {
    $fields['fid'] = $file_upload;
    $file = file_load($file_upload);
    $filename = $file->filename;
    $fields['filename'] = $filename;
    // Move the file out of the user upload directory that the Tripal
    // upload tool uses and into a new directory that is site-specific
    // rather than user-specific.
    $site_dir = tripal_galaxy_get_files_dir();
    if (!$site_dir) {
      $message = 'Could not access the directory on the server for storing this file.';
      drupal_json_output(array(
        'status'  => 'failed',
        'message' => $message,
        'file_id' => '',
      ));
      return;
    }
    file_move($file, $site_dir . '/' . $filename);
  }


Delete all histories from Galaxy that are older than a specified age
--------------------------------------------------------------------

Within Tripal Galaxy (admin/tripal/extension/galaxy/settings) a maximum history age can be set. The default age is 60 days once histories are older than that they will be deleted from the remote Galaxy server and the local workflow invocation status will be changed to 'Deleted'. 

.. code-block:: php

  /**
  * Walks through the tripal_galaxy_workflow_submission table and deletes any 
  * workflows older than specifed in the tripal_galaxy_history_age system 
  * variable.
  *
  *
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


Delete a single history from Galaxy
----------------------------------- 

If a single history needs to be deleted from a remote Galaxy server this function should be used. It does not update the status of the workflow submission in the tripal_galaxy_worklow_submission table so it's important when calling this that table is updated to either completely remove that submission or update the submission status.

.. code-block:: php

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
