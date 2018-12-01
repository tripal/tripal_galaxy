Tripal Galaxy API
==============================

.. note::

  Use of these API functions is predicated on the installation of the Tripal Galaxy module and the blend4php library.

The Tripal Galaxy API allows you to integration Tripal and Galaxy and customize some or all of the process.  By default the Tripal Galaxy module provides an interface that allows a site to offer a step-by-step interface for a workflow, maintaining the look-and-feel of the site.  If that functionality is sufficient you will not need this API documentation.

The blend4php library was specifically built to provide a native PHP interface to the Galaxy API such that any PHP application can connect to the RESTful web services of a remote Galaxy server.  While blend4php provides much of the functions needed, the Tripal Galaxy module does provide a few other functions that help with Tripal integration.  This document describes those functions.

Connect to Galaxy
-----------------
Before any communication between Tripal and Galaxy can happen a connection between the two must be made.  Blend4php will allow you to connnect directly to a Galaxy instance if you know the URL. However, it is recommended that Tripal modules interact with the Tripal Galaxy module API to ensure consistency.  

If the site admin has already added a Galaxy server using the web interface then you can connect to the Galaxy server by knowing it's internal ID.  You can query the `tripal_galaxy` table and retrieving the value of the `galaxy_id` column. Next, the `tripal_galaxy_get_connection` function can be used. It returns an instance of a  `GalaxyInstance`. 

For example:

.. code-block:: php

  // Here we hard-code the server name as 'Local Galaxy'.  This is just an
  // example. In practice you would provide the name of the desired Galaxy
  // server.
  $galaxy_id = db_select('tripal_galaxy', 'tg')
     ->fields('tg', ['galaxy_id'])
     ->condition('servername', 'Local Galaxy')
     ->execute()
     ->fetchField();

  // Connect to the Galaxy instance.
  $galaxy = tripal_galaxy_get_connection($galaxy_id);
  if (!$galaxy) {
    $error = $galaxy->getError();
    drupal_set_message('Could not connect to Galaxy server. ' . $error['message'], 'error');
    return false;
  }

Test if a Galaxy server is accessible.
--------------------------------------
If a Galaxy server is not accessible no actions can be performed including workflow submissions, status updates, or results display. You can check the status of a galaxy workflow with the `tripal_galaxy_test_connection` function. For example, using the `$galaxy_id` obtained in the previous code example:

.. code-block:: php

  $server_status = tripal_galaxy_test_connection(['galaxy_id' => galaxy_id]);


The GalaxyInstance Object
-------------------------
In the example code above above, the `tripal_galaxy_get_connection` function returns an instance of the `GalaxyInstance` class. This instance is used for all other functions used by blend4php to interact with Galaxy.  Here we not provide instructions for using the blend4php library. But, as a brief example, The `GalaxyInstnace` is used with the `GalaxyWorflows` class to retrieve a list of workflows. The example below shows this and an  also how to retrieve a the list of past invocations of that workflow.
  
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

Import a Workflow
-----------------
A site administrator can add new workflows to the Tripal using the Tripal Galaxy admin interface.  However, you can programmatically add a new workflow using the `tripal_galaxy_import_workflow` function.  To do this you must know have the ID of the galaxy server and the name of or workflow_id of the workflow.  You can obtain the name and the workflow_id from the Galaxy server. The following code example shows how to use this function:

.. code-block:: php

  $values = [
     'workflow_id' => 'ebfb8f50c6abde6d',
  ];
  $workflow = tripal_galaxy_add_workflow($galaxy_id, $values, TRUE);

The first argument to the `tripal_galaxy_add_workflow` indicates the ID of the galaxy server that houses the workflow.  The second argument provides searh criteria to specify the workflow.  This can use the `workflow_name` or the `workflow_id`. The third argument indicates that a web form should be created such that end-users can submit the workflow for execution using an online form on the Tripal site.  If you do not wish to expose the workflow to the end-user set this to FALSE.

Creating a Workflow Submission
------------------------------
tripal_galaxy_create_submission($workflow, $user)

Get a History Name
------------------
All data in Galaxy is housed in a collection referred to as a "history". Before workflows can be executed, input data must be placed in a history, and after workflow execution, resulting data is found in the history.  For more information about histories in Galaxy you can view the tutorial page at https://galaxyproject.org/tutorials/histories/.

When Tripal Galaxy invokes a workflow within Galaxy it will ensure that each invokation uses a unique history with a unique name.  By default Tripal Galaxy module uses a naming schema for histories: `TG-[UID]-[WID]-[SID]-[Date]`. 

Where 

- `[UID]` is the user ID of the Drupal user who is submitting/submitted the workflow 
- `[WID]` is the Tripal Galaxy module's ID for the workflow 
- `[SID]` is the Tripal Galaxy submission ID for the workflow submisiion and
- `[Date]` is the date that the submission was made.  

For example the following is history name that follows this scheme:  `TG-1-53-19-2018_10_03_09:31:02`

Before invoking a workflow you will need to create the history with the Galaxy server and place input files and create file "collections" in the history. You can automatically generate the history name for your workflow submission using the `tripal_galaxy_get_history_name` function. For example:


.. code-block:: php

  // Retrieve the $submission object using a known submission ID.
  $submission = tripal_galaxy_get_submission($sid);
  
  // Get the history name.
  $history_name = tripal_galaxy_get_history_name($submission);


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

  
Connect to an Unknown Galaxy Server
----------------------------------- 
While it is better to connect using a known server, we show an example for how to connect to an unknown Galaxy server.  This is provided here to demonstrate For this you must know the URL to the remote Galaxy server and the URL must be split into it's parts: host, port, and protocol. The `tripal_galaxy_split_url` can do this for you, and you can then create your own instance of the `GalaxyInstance` class.

Here is an example of this in use:

.. code-block:: php

  $connect = tripal_galaxy_split_url($galaxy_server->url);
  $galaxy = new GalaxyInstance($connect['host'], $connect['port'], $connect['use_https']);
  $galaxy->setAPIKey($galaxy_server->api_key);
  $error = $galaxy->getErrorType();
  if ($error) {
    return FALSE;
  }