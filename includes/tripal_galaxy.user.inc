<?php

/**
 * @file
 * Functions for display of results to the end-user.
 */

/**
 * Retrieves the workflow submission report for the submitting user.
 *
 * @param int $sid
 *   The submission ID.
 *
 * @return string
 *   A string containing the HTML report.
 */
function tripal_galaxy_submission_user_report(int $sid) {
  global $user;

  // Set the breadcrumb.
  $breadcrumb = array();
  $breadcrumb[] = l(t('Home'), '<front>');
  $breadcrumb[] = l($user->name, 'user/' . $user->uid);
  $breadcrumb[] = l(t('Analyses'), 'user/' . $user->uid . '/galaxy-jobs');
  drupal_set_breadcrumb($breadcrumb);

  module_load_include('inc', 'tripal_galaxy', 'includes/tripal_galaxy.admin');

  if ($report = tripal_galaxy_workflow_report($sid)) {
    return $report;
  }

  drupal_not_found();
}
