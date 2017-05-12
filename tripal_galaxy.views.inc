<?php
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
    'help' => t('The submission node for this workflow.'),
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
    'relationship' => array(
      'base' => 'webform_submissions',
      'field' => 'sid',
      'handler' => 'views_handler_relationship',
      'label' => t('The webform submission of this workflow.'),
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

  // Joins
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
    'webform_submissions' => array(
      'left_field' => 'sid',
      'field' => 'sid',
      'type' => 'INNER',
    ),
  );

  return $data;

}

function tripal_galaxy_views_data_alter(&$data) {

//   // Webform submission from node.
//   $data['node']['tripal_galaxy_workflow'] = array(
//     'title' => t('Workflow submission'),
//     'help' => t('Workflow submissions.'),
//     'real field' => 'nid',
//     'relationship' => array(
//       'base' => 'tripal_galaxy_workflow',
//       'base field' => 'nid',
//       'handler' => 'views_handler_relationship',
//       'label' => t('Webform Submission'),
//     ),
//   );
//   $data['node']['table']['join']['tripal_galaxy_workflow'] = array(
//     'field' => 'nid',
//     'left_field' => 'nid',
//     'left_table' => 'tripal_galaxy_workflow',
//   );
//   $data['webform_submissions']['table']['join']['tripal_galaxy_workflow_submission'] = array(
//     'field' => 'sid',
//     'left_field' => 'sid',
//     'left_table' => 'tripal_galaxy_workflow_submission',
//   );
}