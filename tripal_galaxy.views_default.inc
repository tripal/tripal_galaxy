<?php

/**
 * @file
 * Describes default views.
 */

/**
 * Describes default views.
 */
function tripal_galaxy_views_default_views() {
  $views = array();

  $view = tripal_galaxy_views_job_queue();
  $views[$view->name] = $view;

  return $views;
}

/**
 * Describes the jobs administration view.
 *
 * @ingroup tripal
 */
function tripal_galaxy_views_job_queue() {
  $view = new view();
  $view->name = 'galaxy_workflow_submissions';
  $view->description = '';
  $view->tag = 'default';
  $view->base_table = 'tripal_galaxy_workflow_submission';
  $view->human_name = 'Galaxy Workflow Submissions';
  $view->core = 7;
  $view->api_version = '3.0';
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

  /* Display: Master */
  $handler = $view->new_display('default', 'Master', 'default');
  $handler->display->display_options['title'] = 'Galaxy Workflow Submissions';
  $handler->display->display_options['use_more_always'] = FALSE;
  $handler->display->display_options['access']['type'] = 'perm';
  $handler->display->display_options['access']['perm'] = 'administer galaxy';
  $handler->display->display_options['cache']['type'] = 'none';
  $handler->display->display_options['query']['type'] = 'views_query';
  $handler->display->display_options['exposed_form']['type'] = 'basic';
  $handler->display->display_options['pager']['type'] = 'full';
  $handler->display->display_options['pager']['options']['items_per_page'] = '25';
  $handler->display->display_options['style_plugin'] = 'table';
  $handler->display->display_options['style_options']['columns'] = array(
      'galaxy_workflow_id' => 'galaxy_workflow_id',
      'sid' => 'sid',
      'wf_sid' => 'wf_sid',
      'nid' => 'nid',
      'workflow_name' => 'workflow_name',
      'servername' => 'servername',
      'name' => 'name',
      'submit_date' => 'submit_date',
      'start_time' => 'start_time',
      'end_time' => 'end_time',
      'status' => 'status',
      'view_results' => 'view_results',
      'nothing_1' => 'nothing_1',
      'ctools_dropdown' => 'ctools_dropdown',
  );
  $handler->display->display_options['style_options']['default'] = '-1';
  $handler->display->display_options['style_options']['info'] = array(
      'galaxy_workflow_id' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'sid' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'wf_sid' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'nid' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'workflow_name' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'servername' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'name' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'submit_date' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'start_time' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'end_time' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'status' => array(
          'sortable' => 0,
          'default_sort_order' => 'asc',
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'view_results' => array(
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'nothing_1' => array(
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
      'ctools_dropdown' => array(
          'align' => '',
          'separator' => '',
          'empty_column' => 0,
      ),
  );
  $handler->display->display_options['style_options']['sticky'] = TRUE;
  $handler->display->display_options['style_options']['empty_table'] = TRUE;
  /* Header: Global: Text area */
  $handler->display->display_options['header']['area']['id'] = 'area';
  $handler->display->display_options['header']['area']['table'] = 'views';
  $handler->display->display_options['header']['area']['field'] = 'area';
  $handler->display->display_options['header']['area']['empty'] = TRUE;
  $handler->display->display_options['header']['area']['content'] = 'The following table provides a list of analytical workflows that you have submitted for execution on this site.  Click the drop down in the Actions column to view results, view your submission, re-run or delete it.';
  $handler->display->display_options['header']['area']['format'] = 'filtered_html';
  /* No results behavior: Global: Unfiltered text */
  $handler->display->display_options['empty']['area_text_custom']['id'] = 'area_text_custom';
  $handler->display->display_options['empty']['area_text_custom']['table'] = 'views';
  $handler->display->display_options['empty']['area_text_custom']['field'] = 'area_text_custom';
  $handler->display->display_options['empty']['area_text_custom']['label'] = 'No Results';
  $handler->display->display_options['empty']['area_text_custom']['empty'] = TRUE;
  $handler->display->display_options['empty']['area_text_custom']['content'] = 'There are no workflows for the given filter criteria.';
  /* Relationship: Galaxy Workflow Submission: Submitting User */
  $handler->display->display_options['relationships']['uid']['id'] = 'uid';
  $handler->display->display_options['relationships']['uid']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['relationships']['uid']['field'] = 'uid';
  $handler->display->display_options['relationships']['uid']['label'] = 'User';
  /* Relationship: Galaxy Workflow Submission: The Galaxy Workflow */
  $handler->display->display_options['relationships']['galaxy_workflow_id']['id'] = 'galaxy_workflow_id';
  $handler->display->display_options['relationships']['galaxy_workflow_id']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['relationships']['galaxy_workflow_id']['field'] = 'galaxy_workflow_id';
  $handler->display->display_options['relationships']['galaxy_workflow_id']['label'] = 'Workflow';
  /* Field: Galaxy Workflow: Internal ID */
  $handler->display->display_options['fields']['galaxy_workflow_id']['id'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['galaxy_workflow_id']['table'] = 'tripal_galaxy_workflow';
  $handler->display->display_options['fields']['galaxy_workflow_id']['field'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['galaxy_workflow_id']['relationship'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['galaxy_workflow_id']['exclude'] = TRUE;
  /* Field: Galaxy Workflow Submission: Submission ID */
  $handler->display->display_options['fields']['sid']['id'] = 'sid';
  $handler->display->display_options['fields']['sid']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['sid']['field'] = 'sid';
  $handler->display->display_options['fields']['sid']['label'] = 'ID';
  /* Field: Galaxy Workflow to Webform Submission: Webform Submission ID */
  $handler->display->display_options['fields']['wf_sid']['id'] = 'wf_sid';
  $handler->display->display_options['fields']['wf_sid']['table'] = 'tripal_galaxy_webform';
  $handler->display->display_options['fields']['wf_sid']['field'] = 'wf_sid';
  $handler->display->display_options['fields']['wf_sid']['label'] = '';
  $handler->display->display_options['fields']['wf_sid']['exclude'] = TRUE;
  $handler->display->display_options['fields']['wf_sid']['element_label_colon'] = FALSE;
  /* Field: Galaxy Workflow: Node */
  $handler->display->display_options['fields']['nid']['id'] = 'nid';
  $handler->display->display_options['fields']['nid']['table'] = 'tripal_galaxy_workflow';
  $handler->display->display_options['fields']['nid']['field'] = 'nid';
  $handler->display->display_options['fields']['nid']['relationship'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['nid']['label'] = '';
  $handler->display->display_options['fields']['nid']['exclude'] = TRUE;
  $handler->display->display_options['fields']['nid']['element_label_colon'] = FALSE;
  /* Field: User: Uid */
  $handler->display->display_options['fields']['uid']['id'] = 'uid';
  $handler->display->display_options['fields']['uid']['table'] = 'users';
  $handler->display->display_options['fields']['uid']['field'] = 'uid';
  $handler->display->display_options['fields']['uid']['relationship'] = 'uid';
  $handler->display->display_options['fields']['uid']['label'] = '';
  $handler->display->display_options['fields']['uid']['exclude'] = TRUE;
  $handler->display->display_options['fields']['uid']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['uid']['link_to_user'] = FALSE;
  /* Field: Galaxy Workflow: Workflow Name */
  $handler->display->display_options['fields']['workflow_name']['id'] = 'workflow_name';
  $handler->display->display_options['fields']['workflow_name']['table'] = 'tripal_galaxy_workflow';
  $handler->display->display_options['fields']['workflow_name']['field'] = 'workflow_name';
  $handler->display->display_options['fields']['workflow_name']['relationship'] = 'galaxy_workflow_id';
  /* Field: Galaxy Server: Server Name */
  $handler->display->display_options['fields']['servername']['id'] = 'servername';
  $handler->display->display_options['fields']['servername']['table'] = 'tripal_galaxy';
  $handler->display->display_options['fields']['servername']['field'] = 'servername';
  $handler->display->display_options['fields']['servername']['relationship'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['servername']['label'] = 'Galaxy Server';
  /* Field: User: Name */
  $handler->display->display_options['fields']['name']['id'] = 'name';
  $handler->display->display_options['fields']['name']['table'] = 'users';
  $handler->display->display_options['fields']['name']['field'] = 'name';
  $handler->display->display_options['fields']['name']['relationship'] = 'uid';
  /* Field: Galaxy Workflow Submission: Submission Time */
  $handler->display->display_options['fields']['submit_date']['id'] = 'submit_date';
  $handler->display->display_options['fields']['submit_date']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['submit_date']['field'] = 'submit_date';
  $handler->display->display_options['fields']['submit_date']['date_format'] = 'short';
  $handler->display->display_options['fields']['submit_date']['second_date_format'] = 'long';
  /* Field: Galaxy Workflow Submission: Start Time */
  $handler->display->display_options['fields']['start_time']['id'] = 'start_time';
  $handler->display->display_options['fields']['start_time']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['start_time']['field'] = 'start_time';
  $handler->display->display_options['fields']['start_time']['date_format'] = 'short';
  $handler->display->display_options['fields']['start_time']['second_date_format'] = 'long';
  /* Field: Galaxy Workflow Submission: End Time */
  $handler->display->display_options['fields']['end_time']['id'] = 'end_time';
  $handler->display->display_options['fields']['end_time']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['end_time']['field'] = 'end_time';
  $handler->display->display_options['fields']['end_time']['date_format'] = 'short';
  $handler->display->display_options['fields']['end_time']['second_date_format'] = 'long';
  /* Field: Galaxy Workflow Submission: Submission Status */
  $handler->display->display_options['fields']['status']['id'] = 'status';
  $handler->display->display_options['fields']['status']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['status']['field'] = 'status';
  $handler->display->display_options['fields']['status']['label'] = 'Status';
  /* Field: Galaxy Workflow Submission: View Results Link */
  $handler->display->display_options['fields']['view_results']['id'] = 'view_results';
  $handler->display->display_options['fields']['view_results']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['view_results']['field'] = 'view_results';
  $handler->display->display_options['fields']['view_results']['relationship'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['view_results']['label'] = '';
  $handler->display->display_options['fields']['view_results']['exclude'] = TRUE;
  $handler->display->display_options['fields']['view_results']['alter']['make_link'] = TRUE;
  $handler->display->display_options['fields']['view_results']['alter']['path'] = 'user/[uid]/galaxy-jobs/[sid]';
  $handler->display->display_options['fields']['view_results']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['view_results']['hide_empty'] = TRUE;
  $handler->display->display_options['fields']['view_results']['hide_alter_empty'] = FALSE;
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing_1']['id'] = 'nothing_1';
  $handler->display->display_options['fields']['nothing_1']['table'] = 'views';
  $handler->display->display_options['fields']['nothing_1']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing_1']['label'] = 'View Submission';
  $handler->display->display_options['fields']['nothing_1']['exclude'] = TRUE;
  $handler->display->display_options['fields']['nothing_1']['alter']['text'] = 'View Submission';
  $handler->display->display_options['fields']['nothing_1']['alter']['make_link'] = TRUE;
  $handler->display->display_options['fields']['nothing_1']['alter']['path'] = 'node/[nid]/submission/[wf_sid]';
  /* Field: Global: Dropdown links */
  $handler->display->display_options['fields']['ctools_dropdown']['id'] = 'ctools_dropdown';
  $handler->display->display_options['fields']['ctools_dropdown']['table'] = 'views';
  $handler->display->display_options['fields']['ctools_dropdown']['field'] = 'ctools_dropdown';
  $handler->display->display_options['fields']['ctools_dropdown']['label'] = 'Actions';
  $handler->display->display_options['fields']['ctools_dropdown']['fields'] = array(
      'view_results' => 'view_results',
      'nothing_1' => 'nothing_1',
  );
  $handler->display->display_options['fields']['ctools_dropdown']['check_access'] = 0;
  /* Sort criterion: Galaxy Workflow Submission: Submission Time */
  $handler->display->display_options['sorts']['submit_date']['id'] = 'submit_date';
  $handler->display->display_options['sorts']['submit_date']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['sorts']['submit_date']['field'] = 'submit_date';
  $handler->display->display_options['sorts']['submit_date']['order'] = 'DESC';
  $handler->display->display_options['sorts']['submit_date']['expose']['label'] = 'Submission Time';
  /* Filter criterion: Galaxy Workflow Submission: Submission Status */
  $handler->display->display_options['filters']['status']['id'] = 'status';
  $handler->display->display_options['filters']['status']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['filters']['status']['field'] = 'status';
  $handler->display->display_options['filters']['status']['exposed'] = TRUE;
  $handler->display->display_options['filters']['status']['expose']['operator_id'] = 'status_op';
  $handler->display->display_options['filters']['status']['expose']['label'] = 'Submission Status';
  $handler->display->display_options['filters']['status']['expose']['operator'] = 'status_op';
  $handler->display->display_options['filters']['status']['expose']['identifier'] = 'status';
  $handler->display->display_options['filters']['status']['is_grouped'] = TRUE;
  $handler->display->display_options['filters']['status']['group_info']['label'] = 'Submission Status';
  $handler->display->display_options['filters']['status']['group_info']['identifier'] = 'status';
  $handler->display->display_options['filters']['status']['group_info']['default_group'] = '1';
  $handler->display->display_options['filters']['status']['group_info']['group_items'] = array(
      1 => array(
          'title' => '- Active -',
          'operator' => 'word',
          'value' => 'Completed Waiting Processing Error',
      ),
      2 => array(
          'title' => 'Completed',
          'operator' => '=',
          'value' => 'Completed',
      ),
      3 => array(
          'title' => 'Processing',
          'operator' => '=',
          'value' => 'Processing',
      ),
      4 => array(
          'title' => 'Waiting',
          'operator' => '=',
          'value' => 'Waiting',
      ),
      5 => array(
          'title' => 'Error',
          'operator' => '=',
          'value' => 'Error',
      ),
      6 => array(
          'title' => 'Deleted',
          'operator' => '=',
          'value' => 'Deleted',
      ),
  );
  /* Filter criterion: Galaxy Workflow: Workflow Name */
  $handler->display->display_options['filters']['workflow_name']['id'] = 'workflow_name';
  $handler->display->display_options['filters']['workflow_name']['table'] = 'tripal_galaxy_workflow';
  $handler->display->display_options['filters']['workflow_name']['field'] = 'workflow_name';
  $handler->display->display_options['filters']['workflow_name']['relationship'] = 'galaxy_workflow_id';
  $handler->display->display_options['filters']['workflow_name']['operator'] = 'word';
  $handler->display->display_options['filters']['workflow_name']['exposed'] = TRUE;
  $handler->display->display_options['filters']['workflow_name']['expose']['operator_id'] = 'workflow_name_op';
  $handler->display->display_options['filters']['workflow_name']['expose']['label'] = 'Workflow Name';
  $handler->display->display_options['filters']['workflow_name']['expose']['operator'] = 'workflow_name_op';
  $handler->display->display_options['filters']['workflow_name']['expose']['identifier'] = 'workflow_name';
  $handler->display->display_options['filters']['workflow_name']['expose']['remember_roles'] = array(
      2 => '2',
      1 => 0,
      3 => 0,
  );
  /* Filter criterion: User: Name */
  $handler->display->display_options['filters']['uid']['id'] = 'uid';
  $handler->display->display_options['filters']['uid']['table'] = 'users';
  $handler->display->display_options['filters']['uid']['field'] = 'uid';
  $handler->display->display_options['filters']['uid']['relationship'] = 'uid';
  $handler->display->display_options['filters']['uid']['value'] = '';
  $handler->display->display_options['filters']['uid']['exposed'] = TRUE;
  $handler->display->display_options['filters']['uid']['expose']['operator_id'] = 'uid_op';
  $handler->display->display_options['filters']['uid']['expose']['label'] = 'User';
  $handler->display->display_options['filters']['uid']['expose']['operator'] = 'uid_op';
  $handler->display->display_options['filters']['uid']['expose']['identifier'] = 'uid';
  $handler->display->display_options['filters']['uid']['expose']['remember_roles'] = array(
      2 => '2',
      1 => 0,
      3 => 0,
  );
  $handler->display->display_options['filters']['uid']['group_info']['label'] = 'Name';
  $handler->display->display_options['filters']['uid']['group_info']['identifier'] = 'uid';
  $handler->display->display_options['filters']['uid']['group_info']['remember'] = FALSE;
  $handler->display->display_options['filters']['uid']['group_info']['group_items'] = array(
      1 => array(),
      2 => array(),
      3 => array(),
  );

  /* Display: Admin Galaxy Job Queue */
  $handler = $view->new_display('page', 'Admin Galaxy Job Queue', 'page');
  $handler->display->display_options['path'] = 'admin/tripal/extension/galaxy/job-queue';
  $handler->display->display_options['menu']['type'] = 'tab';
  $handler->display->display_options['menu']['title'] = 'Job Queue';
  $handler->display->display_options['menu']['description'] = 'The Galaxy Job Queue';
  $handler->display->display_options['menu']['weight'] = '15';
  $handler->display->display_options['menu']['context'] = 0;
  $handler->display->display_options['menu']['context_only_inline'] = 0;

  /* Display: User Galaxy Job Queue */
  $handler = $view->new_display('page', 'User Galaxy Job Queue', 'page_1');
  $handler->display->display_options['defaults']['access'] = FALSE;
  $handler->display->display_options['access']['type'] = 'perm';
  $handler->display->display_options['access']['perm'] = 'use galaxy';
  $handler->display->display_options['defaults']['fields'] = FALSE;
  /* Field: Galaxy Workflow: Internal ID */
  $handler->display->display_options['fields']['galaxy_workflow_id']['id'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['galaxy_workflow_id']['table'] = 'tripal_galaxy_workflow';
  $handler->display->display_options['fields']['galaxy_workflow_id']['field'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['galaxy_workflow_id']['relationship'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['galaxy_workflow_id']['exclude'] = TRUE;
  /* Field: Galaxy Workflow Submission: Submission ID */
  $handler->display->display_options['fields']['sid']['id'] = 'sid';
  $handler->display->display_options['fields']['sid']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['sid']['field'] = 'sid';
  $handler->display->display_options['fields']['sid']['label'] = 'ID';
  $handler->display->display_options['fields']['sid']['exclude'] = TRUE;
  /* Field: Galaxy Workflow to Webform Submission: Webform Submission ID */
  $handler->display->display_options['fields']['wf_sid']['id'] = 'wf_sid';
  $handler->display->display_options['fields']['wf_sid']['table'] = 'tripal_galaxy_webform';
  $handler->display->display_options['fields']['wf_sid']['field'] = 'wf_sid';
  $handler->display->display_options['fields']['wf_sid']['label'] = '';
  $handler->display->display_options['fields']['wf_sid']['exclude'] = TRUE;
  $handler->display->display_options['fields']['wf_sid']['element_label_colon'] = FALSE;
  /* Field: Galaxy Workflow: Node */
  $handler->display->display_options['fields']['nid']['id'] = 'nid';
  $handler->display->display_options['fields']['nid']['table'] = 'tripal_galaxy_workflow';
  $handler->display->display_options['fields']['nid']['field'] = 'nid';
  $handler->display->display_options['fields']['nid']['relationship'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['nid']['label'] = '';
  $handler->display->display_options['fields']['nid']['exclude'] = TRUE;
  $handler->display->display_options['fields']['nid']['element_label_colon'] = FALSE;
  /* Field: Galaxy Workflow: Workflow Name */
  $handler->display->display_options['fields']['workflow_name']['id'] = 'workflow_name';
  $handler->display->display_options['fields']['workflow_name']['table'] = 'tripal_galaxy_workflow';
  $handler->display->display_options['fields']['workflow_name']['field'] = 'workflow_name';
  $handler->display->display_options['fields']['workflow_name']['relationship'] = 'galaxy_workflow_id';
  /* Field: Galaxy Server: Server Name */
  $handler->display->display_options['fields']['servername']['id'] = 'servername';
  $handler->display->display_options['fields']['servername']['table'] = 'tripal_galaxy';
  $handler->display->display_options['fields']['servername']['field'] = 'servername';
  $handler->display->display_options['fields']['servername']['relationship'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['servername']['label'] = 'Galaxy Server';
  $handler->display->display_options['fields']['servername']['exclude'] = TRUE;
  /* Field: User: Name */
  $handler->display->display_options['fields']['name']['id'] = 'name';
  $handler->display->display_options['fields']['name']['table'] = 'users';
  $handler->display->display_options['fields']['name']['field'] = 'name';
  $handler->display->display_options['fields']['name']['relationship'] = 'uid';
  $handler->display->display_options['fields']['name']['exclude'] = TRUE;
  /* Field: User: Uid */
  $handler->display->display_options['fields']['uid']['id'] = 'uid';
  $handler->display->display_options['fields']['uid']['table'] = 'users';
  $handler->display->display_options['fields']['uid']['field'] = 'uid';
  $handler->display->display_options['fields']['uid']['relationship'] = 'uid';
  $handler->display->display_options['fields']['uid']['label'] = '';
  $handler->display->display_options['fields']['uid']['exclude'] = TRUE;
  $handler->display->display_options['fields']['uid']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['uid']['link_to_user'] = FALSE;
  /* Field: Galaxy Workflow Submission: Submission Time */
  $handler->display->display_options['fields']['submit_date']['id'] = 'submit_date';
  $handler->display->display_options['fields']['submit_date']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['submit_date']['field'] = 'submit_date';
  $handler->display->display_options['fields']['submit_date']['date_format'] = 'short';
  $handler->display->display_options['fields']['submit_date']['second_date_format'] = 'long';
  /* Field: Galaxy Workflow Submission: Start Time */
  $handler->display->display_options['fields']['start_time']['id'] = 'start_time';
  $handler->display->display_options['fields']['start_time']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['start_time']['field'] = 'start_time';
  $handler->display->display_options['fields']['start_time']['date_format'] = 'short';
  $handler->display->display_options['fields']['start_time']['second_date_format'] = 'long';
  /* Field: Galaxy Workflow Submission: End Time */
  $handler->display->display_options['fields']['end_time']['id'] = 'end_time';
  $handler->display->display_options['fields']['end_time']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['end_time']['field'] = 'end_time';
  $handler->display->display_options['fields']['end_time']['date_format'] = 'short';
  $handler->display->display_options['fields']['end_time']['second_date_format'] = 'long';
  /* Field: Galaxy Workflow Submission: Submission Status */
  $handler->display->display_options['fields']['status']['id'] = 'status';
  $handler->display->display_options['fields']['status']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['status']['field'] = 'status';
  $handler->display->display_options['fields']['status']['label'] = 'Status';
  /* Field: Galaxy Workflow Submission: View Results Link */
  $handler->display->display_options['fields']['view_results']['id'] = 'view_results';
  $handler->display->display_options['fields']['view_results']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['fields']['view_results']['field'] = 'view_results';
  $handler->display->display_options['fields']['view_results']['relationship'] = 'galaxy_workflow_id';
  $handler->display->display_options['fields']['view_results']['label'] = '';
  $handler->display->display_options['fields']['view_results']['exclude'] = TRUE;
  $handler->display->display_options['fields']['view_results']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['view_results']['hide_empty'] = TRUE;
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing_1']['id'] = 'nothing_1';
  $handler->display->display_options['fields']['nothing_1']['table'] = 'views';
  $handler->display->display_options['fields']['nothing_1']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing_1']['label'] = 'View Submission';
  $handler->display->display_options['fields']['nothing_1']['exclude'] = TRUE;
  $handler->display->display_options['fields']['nothing_1']['alter']['text'] = 'View Submission';
  $handler->display->display_options['fields']['nothing_1']['alter']['make_link'] = TRUE;
  $handler->display->display_options['fields']['nothing_1']['alter']['path'] = 'node/[nid]/submission/[wf_sid]';
  $handler->display->display_options['fields']['nothing_1']['hide_empty'] = TRUE;
  $handler->display->display_options['fields']['nothing_1']['hide_alter_empty'] = TRUE;
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing']['id'] = 'nothing';
  $handler->display->display_options['fields']['nothing']['table'] = 'views';
  $handler->display->display_options['fields']['nothing']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing']['label'] = 'Re-run';
  $handler->display->display_options['fields']['nothing']['exclude'] = TRUE;
  $handler->display->display_options['fields']['nothing']['alter']['text'] = 'Re-run';
  $handler->display->display_options['fields']['nothing']['alter']['make_link'] = TRUE;
  $handler->display->display_options['fields']['nothing']['alter']['path'] = 'node/[nid]';
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing_2']['id'] = 'nothing_2';
  $handler->display->display_options['fields']['nothing_2']['table'] = 'views';
  $handler->display->display_options['fields']['nothing_2']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing_2']['label'] = 'Delete';
  $handler->display->display_options['fields']['nothing_2']['exclude'] = TRUE;
  $handler->display->display_options['fields']['nothing_2']['alter']['text'] = 'Delete';
  $handler->display->display_options['fields']['nothing_2']['alter']['make_link'] = TRUE;
  $handler->display->display_options['fields']['nothing_2']['alter']['path'] = 'node/[nid]/submission/[wf_sid]/delete?destination=user/[uid]/galaxy-jobs';
  /* Field: Global: Dropdown links */
  $handler->display->display_options['fields']['ctools_dropdown']['id'] = 'ctools_dropdown';
  $handler->display->display_options['fields']['ctools_dropdown']['table'] = 'views';
  $handler->display->display_options['fields']['ctools_dropdown']['field'] = 'ctools_dropdown';
  $handler->display->display_options['fields']['ctools_dropdown']['label'] = 'Actions';
  $handler->display->display_options['fields']['ctools_dropdown']['fields'] = array(
      'view_results' => 'view_results',
      'nothing_1' => 'nothing_1',
      'nothing' => 'nothing',
      'nothing_2' => 'nothing_2',
  );
  $handler->display->display_options['fields']['ctools_dropdown']['check_access'] = 0;
  $handler->display->display_options['fields']['ctools_dropdown']['destination'] = FALSE;
  $handler->display->display_options['defaults']['filter_groups'] = FALSE;
  $handler->display->display_options['defaults']['filters'] = FALSE;
  /* Filter criterion: Galaxy Workflow Submission: Submission Status */
  $handler->display->display_options['filters']['status']['id'] = 'status';
  $handler->display->display_options['filters']['status']['table'] = 'tripal_galaxy_workflow_submission';
  $handler->display->display_options['filters']['status']['field'] = 'status';
  $handler->display->display_options['filters']['status']['exposed'] = TRUE;
  $handler->display->display_options['filters']['status']['expose']['operator_id'] = 'status_op';
  $handler->display->display_options['filters']['status']['expose']['label'] = 'Submission Status';
  $handler->display->display_options['filters']['status']['expose']['operator'] = 'status_op';
  $handler->display->display_options['filters']['status']['expose']['identifier'] = 'status';
  $handler->display->display_options['filters']['status']['is_grouped'] = TRUE;
  $handler->display->display_options['filters']['status']['group_info']['label'] = 'Submission Status';
  $handler->display->display_options['filters']['status']['group_info']['identifier'] = 'status';
  $handler->display->display_options['filters']['status']['group_info']['default_group'] = '1';
  $handler->display->display_options['filters']['status']['group_info']['group_items'] = array(
      1 => array(
          'title' => '- Active -',
          'operator' => 'word',
          'value' => 'Completed Waiting Processing Error Submitted',
      ),
      2 => array(
          'title' => 'Completed',
          'operator' => '=',
          'value' => 'Completed',
      ),
      3 => array(
          'title' => 'Processing',
          'operator' => '=',
          'value' => 'Processing',
      ),
      4 => array(
          'title' => 'Submitted',
          'operator' => '=',
          'value' => 'Submitted',
      ),
      5 => array(
          'title' => 'Waiting',
          'operator' => '=',
          'value' => 'Waiting',
      ),
      6 => array(
          'title' => 'Error',
          'operator' => '=',
          'value' => 'Error',
      ),
      7 => array(
          'title' => 'Deleted',
          'operator' => '=',
          'value' => 'Deleted',
      ),
  );
  /* Filter criterion: Galaxy Workflow: Workflow Name */
  $handler->display->display_options['filters']['workflow_name']['id'] = 'workflow_name';
  $handler->display->display_options['filters']['workflow_name']['table'] = 'tripal_galaxy_workflow';
  $handler->display->display_options['filters']['workflow_name']['field'] = 'workflow_name';
  $handler->display->display_options['filters']['workflow_name']['relationship'] = 'galaxy_workflow_id';
  $handler->display->display_options['filters']['workflow_name']['operator'] = 'word';
  $handler->display->display_options['filters']['workflow_name']['exposed'] = TRUE;
  $handler->display->display_options['filters']['workflow_name']['expose']['operator_id'] = 'workflow_name_op';
  $handler->display->display_options['filters']['workflow_name']['expose']['label'] = 'Workflow Name';
  $handler->display->display_options['filters']['workflow_name']['expose']['operator'] = 'workflow_name_op';
  $handler->display->display_options['filters']['workflow_name']['expose']['identifier'] = 'workflow_name';
  $handler->display->display_options['filters']['workflow_name']['expose']['remember_roles'] = array(
      2 => '2',
      1 => 0,
      3 => 0,
  );
  /* Filter criterion: User: Current */
  $handler->display->display_options['filters']['uid_current']['id'] = 'uid_current';
  $handler->display->display_options['filters']['uid_current']['table'] = 'users';
  $handler->display->display_options['filters']['uid_current']['field'] = 'uid_current';
  $handler->display->display_options['filters']['uid_current']['relationship'] = 'uid';
  $handler->display->display_options['filters']['uid_current']['value'] = '1';
  $handler->display->display_options['path'] = 'user/%/galaxy-jobs';
  $handler->display->display_options['menu']['type'] = 'tab';
  $handler->display->display_options['menu']['title'] = 'Analyses';
  $handler->display->display_options['menu']['description'] = 'Sumitted Workflow Analyses';
  $handler->display->display_options['menu']['weight'] = '0';
  $handler->display->display_options['menu']['name'] = 'user-menu';
  $handler->display->display_options['menu']['context'] = 0;
  $handler->display->display_options['menu']['context_only_inline'] = 0;

  return $view;
}
