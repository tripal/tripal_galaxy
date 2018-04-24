<?php
/**
 * @file
 * Contains function relating to drush-integration of this module.
 */

/**
 * @defgroup tripal_drush Tripal Drush Integration
 * @{
 * Contains function relating to drush-integration of various tripal modules.
 * @}
 */

/**
 * Describes each drush command implemented by the module
 *
 * @return
 *   The first line of description when executing the help for a given command
 *
 * @ingroup tripal_drush
 */
function tripal_galaxy_drush_help($command) {
  switch ($command) {

    case 'trp-galaxy-invoke':
      return dt('Launches pending galaxy jobs waiting in the queue.');
      break;
    case 'trp-galaxy-status':
      return dt('Launches pending galaxy jobs waiting in the queue.');
      break;
  }
}


function tripal_galaxy_drush_command() {
  $items = array();

  $items['trp-galaxy-invoke'] = array(
    'description' => dt('Launches pending galaxy jobs waiting in the queue.'),
    'examples' => array(
      'Single Job' => 'drush trp-galaxy-invoke --submission=512',
    ),
    'arguments' => array(),
    'options' => array(
      'submission' => dt('The submission ID of the workflow to run.'),
    ),
  );
  $items['trp-galaxy-status'] = array(
    'description' => dt('Updates the status of galaxy jobs.'),
    'examples' => array(
      'Single Job' => 'drush trp-galaxy-status --submission=512',
    ),
    'arguments' => array(),
    'options' => array(
      'submission' => dt('The submission ID of the workflow to run.'),
      'force' => dt('Forces update of status for jobs that are already have a satus of Completed or Error.')
    ),
  );
  return $items;
}

function drush_tripal_galaxy_trp_galaxy_invoke() {
  $sid  = drush_get_option('submission');
  if (!$sid) {
    drush_error('Please provide a submission id (ie.g. --submission=512).');
  }
  drush_print('Launching workflow...');
  try {
    tripal_galaxy_invoke_workflow($sid);
    drush_print('Done.');
  }
  catch (Exception $e) {
    drush_set_error("Cannot lanuch workflow: " . $e->getMessage());
  }
}

function drush_tripal_galaxy_trp_galaxy_status() {
  $sid  = drush_get_option('submission');
  $force  = drush_get_option('force');

  if (!$force) {
    $force = FALSE;
  }
  else {
    $force = TRUE;
  }

  if (!$sid) {
    drush_set_error('Please provide a submission id (ie.g. --submission=512).');
  }
  drush_print('Updating status of workflow...');
  tripal_galaxy_check_submission_status($sid, $force);
  drush_print('Done.');

}


