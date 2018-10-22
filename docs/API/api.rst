Tripal Galaxy API
==============================

.. note::

  Use of these API functions is predicated on the installation of the Tripal Galaxy module and the blend4php library.

The Tripal Galaxy API allows you to plugin to the integration between Tripal and Galaxy and customize some or all of the process.  By default the Tripal Galaxy module provides an interface that allows a site to offer a step-by-step interface for a workflow, maintaining the look-and-feel of the site.  If that functionality is sufficient you will not need this API documentation.


Open a Connection
-----------------
Before any communication between Tripal and Galaxy can happen a connection between the two must be made. The tripal_galaxy_get_connection($galaxy_id) retreives a GalaxyInstance objects using a galaxy_id thereby opening the required communication path.

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
To successfully connect to a Galaxy instance the url must be split into it's parts: host, port, and protocol. The tripal_galaxy_split_url($url) function accepts a $url variable and returns its part.

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
Communication between Galaxy and Tripal needs to be initiated and specific information needs to be requested. Given that, the tripal_galaxy_check_submission_status($sid, $force = FALSE) function checks the status of a Galaxy workflow and updates the status tripal_galaxy_workflow_submission table with the results. A workflow on the Tripal Galaxy side will have one of 4 statuses: Waiting, Submitted, Completed or Error. 

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

When Tripal Galaxy creates a workflow within Galaxy it structures the History name as: "TG-NodeId-GalaxyWorflowID-SubmissionID-DateTimeOfSubmission". So the function tripal_galaxy_get_history_name($submission, $node) can build the name with just the $submission and $node objects. 

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

Invoking a workflow using this api function, tripal_galaxy_invoke_workflow($galaxy, $workflow_id, $parameters, $inputs, $history_id, $sid), requires a complex array that is mapped directly to the workflow that is being invoked. Tripal Galaxy uses the data returned from the webform submission to build this array, for an example of this you can review tripal_galaxy_invoke_webform_submission in tripal_galaxy.webform.inc. 

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

Here is an example from the function tripal_galaxy_invoke_webform_submission in the tripal_galaxy.webform.inc file of how to use it:

.. code-block:: php
  // Call the Tripal Galaxy API function to invoke this workflow.
  tripal_galaxy_invoke_workflow($galaxy, $submission->workflow_id, $parameters, 
    $input_datasets, $history['id'], $sid);

Upload a file to Galaxy
-----------------------

For loading files from your local Tripal site into Galaxy use the tripal_galaxy_upload_file($galaxy, $fid, $history_id, $history_contents) function.

Here is an example of this function in use from the tripal_galaxy_invoke_webform_submission() function in tripal_galaxy.webform.inc file: 

.. code-block:: php
  // Handle a single file upload.
  if ($data->type == 'galaxy_sfile') {
    if ($data->no == 'data_collection' or $data->no == 'existing' or preg_match('/^submitted_/', $data->no)) {
      $fid = $data->data;
      if ($fid) {
        $file = tripal_galaxy_upload_file($galaxy, $fid, $history['id'], $history_contents);
        $inputs[$data->form_key] = $file;
      }
    }
    else {
      if ($data->no == 'site_wide') {
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
  }

Retrieving a history from Galaxy
--------------------------------

The history is the results from the invocation of the workflow. Tripal Galaxy builds history names in a specific format so that histories are easily accessible and renderable within the Tripal Galaxy interface.

This function, tripal_galaxy_get_history(GalaxyInstance $galaxy, $history_name, &$error), is used frequently throughout the Tripal Galaxy module, here is an example of its use:

Here is an example of how to use it:

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

The function tripal_galaxy_test_connection($connect) will allow you to test the server status. 

Here is an example of its use:
  $sql = "SELECT * FROM {tripal_galaxy}";
  $results = db_query($sql);

  while ($result = $results->fetchObject()) {
    $server_status = tripal_galaxy_test_connection(['galaxy_id' => $result->galaxy_id]);
  }


Tripal Galaxy file storage locator
----------------------------------
Tripal Galaxy and Tripal store user files in different locations, this function, tripal_galaxy_get_files_dir(), returns the location of Tripal Galaxy user files. 

Here is an example of use its within the tripal_galaxy.adin_files.inc file, lines 234-253:

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

Within Tripal Galaxy (admin/tripal/extension/galaxy/settings) a maximum history age can be set. The default age is 60 days once histories are older than that they will be deleted from the remote Galaxy server and the local workflow invocation status will be changed to 'Deleted'. To achieve this the function tripal_galaxy_delete_expired_histories() is used.

Here is an example of it being used in the tripal_galaxy.module file in the tripal_galaxy_cron function:

.. code-block:: php
  // Remove old histories from the remote tripal server.
  try {
    tripal_galaxy_delete_expired_histories();
  } catch (Exception $e) {
    watchdog_exception('tripal_galaxy', $e);
    return;
  }


Delete a single history from Galaxy
----------------------------------- 

If a single history needs to be deleted from a remote Galaxy server this function, tripal_galaxy_delete_remote_history($galaxy_id, $history_name), should be used. It does not update the status of the workflow submission in the tripal_galaxy_worklow_submission table so it's important when calling this that table is updated to either completely remove that submission or update the submission status.

Here is an example of it being used in the tripal_galaxy.api.inc file in the tripal_galaxy_delete_expired_histories function:

.. code-block:: php
  while ($old_workflow = $old_workflows->fetchObject()) {
    ...
    $tp_workflow = db_select('tripal_galaxy_workflow', 'tgw')
      ->fields('tgw')
      ->condition('galaxy_workflow_id', $old_workflow->galaxy_workflow_id, '=')
      ->execute()
      ->fetchObject();

    $node = node_load($tp_workflow->nid);
    $history_name = tripal_galaxy_get_history_name($old_workflow, $node);
    $success = tripal_galaxy_delete_remote_history($tp_workflow->galaxy_id, $history_name);
    ...
  }
