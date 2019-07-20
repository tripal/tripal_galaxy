<?php

/**
 * @file
 * Integrates with Drupal Views.
 */

/**
 * Provides information arrays for Views integration.
 */
function tripal_galaxy_views_data() {
  $data['tripal_galaxy']['table']['group'] = t('Galaxy Server');
  $data['tripal_galaxy']['table']['base'] = array(
    'field' => 'galaxy_id',
    'title' => t('Galaxy Server'),
    'help' => t('Galaxy Server'),
  );
  $data['tripal_galaxy']['galaxy_id'] = array(
    'title' => t('Galaxy Host ID'),
    'help' => t('The internal Galaxy host ID.'),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy']['servername'] = array(
    'title' => t('Server Name'),
    'help' => t('The galaxy server name.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy_workflow']['table']['group'] = t('Galaxy Workflow');
  $data['tripal_galaxy_workflow']['table']['base'] = array(
    'field' => 'galaxy_workflow_id',
    'title' => t('Galaxy Workflow'),
    'help' => t('Workflow ID.'),
  );
  $data['tripal_galaxy_workflow']['galaxy_workflow_id'] = array(
    'title' => t('Internal ID'),
    'help' => t('The internal Galaxy workflow ID.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy_workflow']['status'] = array(
    'title' => t('Workflow Status'),
    'help' => t('The status of the workflow.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );
  $data['tripal_galaxy_workflow']['nid'] = array(
    'title' => t('Node'),
    'help' => t('The webform submission node for this workflow. If the workflow was not submitted via the webform then this value will be NULL.'),
    'relationship' => array(
      'base' => 'node',
      'field' => 'nid',
      'handler' => 'views_handler_relationship',
      'label' => t('The submission node (webform).'),
    ),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy_workflow']['workflow_name'] = array(
    'title' => t('Workflow Name'),
    'help' => t('The workflow name as it is on the Galaxy server. This may be different from the workflow node title for the same workflow this site.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy_workflow']['workflow_id'] = array(
    'title' => t('Workflow ID'),
    'help' => t('The ID of the workflow on the Galaxy server.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy_workflow']['galaxy_id'] = array(
    'title' => t('Galaxy Server ID'),
    'help' => t('The galaxy server ID.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy_workflow_submission']['table']['group'] = t('Galaxy Workflow Submission');
  $data['tripal_galaxy_workflow_submission']['table']['base'] = array(
    'field' => 'sid',
    'title' => t('Galaxy Workflow Submission'),
    'help' => t('Galaxy Workflow Submission'),
  );
  $data['tripal_galaxy_workflow_submission']['sid'] = array(
    'title' => t('Submission ID'),
    'help' => t('The submission ID for this workflow.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );
  $data['tripal_galaxy_workflow_submission']['uid'] = array(
      'title' => t('Submitter ID'),
      'help' => t('The user ID of the user that submitted this workflow.'),
      'field' => array(
          'handler' => 'views_handler_field',
          'click sortable' => TRUE,
      ),
      'filter' => array(
          'handler' => 'views_handler_filter_numeric',
      ),
      'sort' => array(
          'handler' => 'views_handler_sort',
      ),
  );

  $data['tripal_galaxy_workflow_submission']['submit_date'] = array(
    'title' => t('Submission Time'),
    'help' => t('The time of submission for this workflow.'),
    'field' => array(
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_date',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy_workflow_submission']['start_time'] = array(
    'title' => t('Start Time'),
    'help' => t('The time the workflow was given to the Galaxy server for execution.'),
    'field' => array(
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_date',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy_workflow_submission']['end_time'] = array(
    'title' => t('End Time'),
    'help' => t('The time that the workfow completed execution on the Galaxy server.'),
    'field' => array(
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_date',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy_workflow_submission']['status'] = array(
    'title' => t('Submission Status'),
    'help' => t('The status of the workflow execution.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['tripal_galaxy_workflow_submission']['uid'] = array(
    'title' => t('Submitting User'),
    'help' => t('The ID of the user submitting the workflow.'),
    'relationship' => array(
      'base' => 'users',
      'field' => 'uid',
      'handler' => 'views_handler_relationship',
      'label' => t('The submitting User'),
    ),
  );

  $data['tripal_galaxy_workflow_submission']['galaxy_workflow_id'] = array(
    'title' => t('The Galaxy Workflow'),
    'help' => t('The ID of the user submitting the workflow.'),
    'relationship' => array(
      'base' => 'tripal_galaxy_workflow',
      'field' => 'galaxy_workflow_id',
      'handler' => 'views_handler_relationship',
      'label' => t('The Galaxy Workflow'),
    ),
  );

  $data['tripal_galaxy_workflow_submission']['view_results'] = array(
    'title' => t('View Results Link'),
    'help' => t('Link to the submission result page.'),
    'field' => array(
      'handler' => 'views_handler_view_results_field',
    ),
  );
//   $data['tripal_galaxy_workflow_submission']['view_submission'] = array(
//     'title' => t('View Submissions Link'),
//     'help' => t('Link to the submission page.'),
//     'field' => array(
//       'handler' => 'views_handler_view_submission_field',
//     ),
//   );
//   $data['tripal_galaxy_workflow_submission']['rerun'] = array(
//     'title' => t('Rerun Link'),
//     'help' => t('Link to re-run the workflow.'),
//     'field' => array(
//       'handler' => 'views_handler_view_rerun_field',
//     ),
//   );
//   $data['tripal_galaxy_workflow_submission']['delete'] = array(
//     'title' => t('Delete Link'),
//     'help' => t('Link to the delete the submission.'),
//     'field' => array(
//       'handler' => 'views_handler_view_delete_field',
//     ),
//   );


  $data['tripal_galaxy_webform']['table']['group'] = t('Galaxy Workflow to Webform Submission');
  $data['tripal_galaxy_webform']['wf_sid'] = array(
    'title' => t('Webform Submission ID'),
    'help' => t('The webform submission ID for this workflow.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );
  $data['tripal_galaxy_webform']['tg_sid'] = array(
    'title' => t('Galaxy Submission ID'),
    'help' => t('The Galaxy submission ID for this workflow.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );


  // Joins.
  $data['tripal_galaxy']['table']['join'] = array(
   'tripal_galaxy_workflow' => array(
     'left_field' => 'galaxy_id',
     'field' => 'galaxy_id',
     'type' => 'INNER',
   ),
  );
  $data['tripal_galaxy_workflow_submission']['table']['join'] = array(
   'tripal_galaxy_workflow' => array(
     'left_field' => 'galaxy_workflow_id',
     'field' => 'galaxy_workflow_id',
     'type' => 'INNER',
   ),
  );
  $data['tripal_galaxy_webform']['table']['join'] = array(
   'tripal_galaxy_workflow_submission' => array(
     'left_field' => 'sid',
     'field' => 'tg_sid',
     'type' => 'LEFT',
   ),
  );
  $data['tripal_galaxy_workflow_submission']['table']['join'] = array(
    'users' => array(
      'left_field' => 'uid',
      'field' => 'uid',
      'type' => 'LEFT',
    ),
  );

  return $data;
}
