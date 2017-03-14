<?php

function tripal_galaxy_admin_usage_page() {

  // set the breadcrumb
  $breadcrumb = array();
  $breadcrumb[] = l('Home', '<front>');
  $breadcrumb[] = l('Administration', 'admin');
  $breadcrumb[] = l('Tripal', 'admin/tripal');
  $breadcrumb[] = l('Extensions', 'admin/tripal/extension');
  $breadcrumb[] = l('Galaxy', 'admin/tripal/extension/galaxy');
  drupal_set_breadcrumb($breadcrumb);

  tripal_add_d3js();
  drupal_add_js(drupal_get_path ('module', 'tripal_galaxy') . '/theme/js/tripal_galaxy.dashboard.js');
  drupal_add_css(drupal_get_path ('module', 'tripal_galaxy') . '/theme/css/tripal_galaxy.dashboard.css');

  $sql = "
    SELECT U.name, count(*) as count
    FROM {tripal_galaxy_workflow_submission} TGWS
      INNER JOIN {webform_submissions} WS ON TGWS.sid = WS.sid
      INNER JOIN {users} U on U.uid = WS.uid
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

  $sql = "
    SELECT N.title as name, count(*) as count
    FROM {tripal_galaxy_workflow_submission} TGWS
      INNER JOIN {tripal_galaxy_workflow} TGW ON TGWS.galaxy_workflow_id = TGW.galaxy_workflow_id
      INNER JOIN {node} N on N.nid = TGW.nid
    GROUP BY N.title
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

  drupal_add_js("var top10users = " . json_encode($top10users) . ";", array('type' => 'inline'));
  drupal_add_js("var top10workflows = " . json_encode($top10workflows) . ";", array('type' => 'inline'));

  $content = "
    <div id=\"tripal-galaxy-usage-users\">
      <h2>Top 10 Users</h2>
      <p>The following chart shows the top 10 users that have submitted workflows.</p>
      <div id=\"tripal-galaxy-usage-users-chart\"></div>
    </div>
    <div id=\"tripal-galaxy-usage-workflows\">
      <h2>Top 10 Workflows</h2>
      <p>The following chart shows the top 10 workflows that have been submitted.</p>
      <div id=\"tripal-galaxy-usage-workflows-chart\"></div>
    </div>
  ";
  return array(
    '#type' => 'markup',
    '#markup' => $content,
  );
}