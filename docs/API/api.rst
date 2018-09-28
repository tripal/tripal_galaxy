Tripal Galaxy API
==============================

.. note::
Use of these API functions is predicated on the installation of the Tripal Galaxy module and the blend4php library.

The Tripal Galaxy API allows you to plugin to the integration between Tripal and Galaxy and customize some or all of the process.


Open a Connection
----------------------
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


Split a url
----------------------
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
----------------------
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
----------------------
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
----------------------  
Workflows are the multistep process through which data is submitted, analysed, processed, and then results provided. Workflows are created in Galaxy, they cannot be created in Tripal Galaxy. One workflow can and probably will have many different histories, a history is the data and results from a workflow.

For more information on creating and editing workflows please see: https://galaxyproject.org/tutorials/g101/#creating-and-editing-a-workflow



