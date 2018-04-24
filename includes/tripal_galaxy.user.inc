<?php

/**
 * Retrieves the workflow submission report for the submitting user.
 *
 * @param $sid
 * @return mixed
 */
function tripal_galaxy_submission_user_report($sid) {
  global $user;

  // Set the breadcrumb.
  $breadcrumb = array();
  $breadcrumb[] = l('Home', '<front>');
  $breadcrumb[] = l($user->name, 'user/' . $user->uid);
  $breadcrumb[] = l('Analysis Results', 'user/' . $user->uid . '/galaxy-jobs');
  drupal_set_breadcrumb($breadcrumb);

  module_load_include('inc', 'tripal_galaxy', 'includes/tripal_galaxy.admin');

  if($report =  tripal_galaxy_workflow_report($sid)) {
    return $report;
  }

  drupal_not_found();
  return;
}
