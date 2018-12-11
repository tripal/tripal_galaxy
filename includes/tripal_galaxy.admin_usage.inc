<?php

/**
 * @file
 * Proivdes usage reports to the site admin.
 */

/**
 * The usage report page.
 */
function tripal_galaxy_admin_usage_page() {

  // Set the breadcrumb.
  $breadcrumb = array();
  $breadcrumb[] = l(t('Home'), '<front>');
  $breadcrumb[] = l(t('Administration'), 'admin');
  $breadcrumb[] = l(t('Tripal'), 'admin/tripal');
  $breadcrumb[] = l(t('Extensions'), 'admin/tripal/extension');
  $breadcrumb[] = l(t('Galaxy'), 'admin/tripal/extension/galaxy');
  drupal_set_breadcrumb($breadcrumb);

  tripal_add_d3js();
  drupal_add_js(drupal_get_path('module', 'tripal_galaxy') . '/theme/js/tripal_galaxy.dashboard.js');
  drupal_add_css(drupal_get_path('module', 'tripal_galaxy') . '/theme/css/tripal_galaxy.dashboard.css');

  // Get some basic stats.
  $headers = array();
  $rows = array();

  // The number of workflow submissions.
  $sql = "
    SELECT count(*) as count
    FROM {tripal_galaxy_workflow_submission} TGWS
  ";
  $result = db_query($sql);
  $rows[] = array(
    array(
      'data' => 'Total submisssions',
      'header' => TRUE,
      'width' => '50%',
    ),
    $result->fetchField(),
  );

  // The total number of hours executed.
  $sql = "
    SELECT count(name) FROM (
    SELECT DISTINCT U.name
    FROM {tripal_galaxy_workflow_submission} TGWS
      INNER JOIN {users} U on U.uid = TGWS.uid ) as users
  ";
  $result = db_query($sql);
  $rows[] = array(
    array(
      'data' => 'Users Submitting Workflows',
      'header' => TRUE,
    ),
    $result->fetchField(),
  );

  // The total number of hours executed.
  $sql = "
    SELECT sum(duration) as total_seconds
    FROM (
      SELECT (end_time - start_time) as duration
      FROM {tripal_galaxy_workflow_submission} TGWS
    ) as worflows
  ";
  $result = db_query($sql);
  $rows[] = array(
    array(
      'data' => 'Total execution time',
      'header' => TRUE,
    ),
    $result->fetchField(),
  );

  $stats = theme_table(array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(
      'id' => 'tripal-galaxy-usage-stats-table',
    ),
    'sticky' => FALSE,
    'caption' => '',
    'colgroups' => array(),
    'empty' => '',
  ));

  // Get the top 10 users.
  $sql = "
    SELECT U.name, count(*) as count
    FROM {tripal_galaxy_workflow_submission} TGWS
      INNER JOIN {users} U on U.uid = TGWS.uid
    GROUP BY U.name
    LIMIT 10 OFFSET 0
  ";
  $results = db_query($sql);
  $top10users = array();
  while ($result = $results->fetchObject()) {
    $top10users[] = array(
      'name' => $result->name,
      'count' => $result->count,
    );
  }

  // Get the top 10 workflows.
  $sql = "
    SELECT TGW.workflow_name, N.title as name, count(*) as count
    FROM {tripal_galaxy_workflow_submission} TGWS
      INNER JOIN {tripal_galaxy_workflow} TGW ON TGWS.galaxy_workflow_id = TGW.galaxy_workflow_id
      LEFT JOIN {node} N on N.nid = TGW.nid
    GROUP BY TGW.workflow_name, N.title
    LIMIT 10 OFFSET 0
  ";
  $results = db_query($sql);
  $top10workflows = array();
  while ($result = $results->fetchObject()) {
    $top10workflows[] = array(
      'name' => $result->name,
      'count' => $result->count,
    );
  }

  drupal_add_js("var top10users = " . json_encode($top10users) . ";", array(
    'type' => 'inline',
  ));
  drupal_add_js("var top10workflows = " . json_encode($top10workflows) . ";", array(
    'type' => 'inline',
  ));

  $content = "
    <div id=\"tripal-galaxy-usage-stats\" class=\"tripal-galaxy-usage-pane\">
      <h2>Usage Stats</h2>
      $stats
    </div>
    <div id=\"tripal-galaxy-usage-users\" class=\"tripal-galaxy-usage-pane\">
      <h2>Top 10 Submitters</h2>
      <p>The following bar chart shows the top 10 users that have submitted workflows.</p>
      <div id=\"tripal-galaxy-usage-users-chart\"></div>
    </div>
    <div id=\"tripal-galaxy-usage-workflows\" class=\"tripal-galaxy-usage-pane\">
      <h2>Top 10 Workflows Submitted</h2>
      <p>The following bar chart shows the top 10 workflows that have been submitted.</p>
      <div id=\"tripal-galaxy-usage-workflows-chart\"></div>
    </div>
  ";
  return array(
    '#type' => 'markup',
    '#markup' => $content,
  );
}
