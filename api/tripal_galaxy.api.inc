<?php

/**
 * Retreives a GalaxyInstance objects using a galaxy_id
 *
 * @param $galaxy_id
 *   The ID of a galaxy server.
 *
 * @return GalaxyInstance
 *   A galaxyInstance object or FALSE on error.
 *
 */
function tripal_get_galaxy_connection($galaxy_id) {
  // Get the galaxy server for this workflow
  $galaxy_server = db_select('tripal_galaxy', 'tg')
    ->fields('tg')
    ->condition('galaxy_id', $galaxy_id)
    ->execute()
    ->fetchObject();

  $library = libraries_load('blend4php');
  $connect = tripal_galaxy_split_galaxy_url($galaxy_server->url);
  $galaxy = new GalaxyInstance($connect['host'], $connect['port'], $connect['use_https']);
  $galaxy->setAPIKey($galaxy_server->api_key);
  $error = $galaxy->getErrorType();
  if ($error) {
    return FALSE;
  }
  return $galaxy;
}

/**
 * Splits a URL to a Galaxy server into the host, port and if HTTPS is required.
 *
 * @param $utl
 *   The URL for the remote galaxy instance.
 *
 * @return
 *   An array with three keys: host, port and use_https.
 */
function tripal_galaxy_split_galaxy_url($url) {

  // First check a URL with a port
  $matches = array();
  if (preg_match('/^(.*)\:\/\/(.+?)\:(\d+)$/', $url, $matches)) {
    $protocol = $matches[1];
    $host = $matches[2];
    $port = $matches[3];
    $use_https = FALSE;
    if ($protocol == 'https') {
      $use_https = TRUE;
    }
  }
  // Next check a URL without a port
  else if (preg_match('/^(.*)\:\/\/(.+?)$/', $url, $matches)) {
    $protocol = $matches[1];
    $host = $matches[2];
    $port = '';
    $use_https = FALSE;
    if ($protocol == 'https') {
      $use_https = TRUE;
    }
  }
  return array(
    'host' => $host,
    'port' => $port,
    'use_https' => $use_https
  );
}

/**
 * Checks and updates the status of a Galaxy workflow.
 *
 * Each Galaxy workflow has a unique UUID.  Each time the workflow is changed
 * that UUID changes as well.
 */
function tripal_check_galaxy_workflow_status($galaxy_workflow_id) {

  if (!$galaxy_workflow_id) {
    return FALSE;
  }
  if (!is_numeric($galaxy_workflow_id)) {
    return FALSE;
  }
  $workflow = db_select('tripal_galaxy_workflow', 'tgw')
    ->fields('tgw')
    ->condition('tgw.galaxy_workflow_id', $galaxy_workflow_id)
    ->execute()
    ->fetchObject();

  // Connect to the Galaxy instance and get the list of workflows
  $galaxy_instance = tripal_get_galaxy_connection($workflow->galaxy_id);
  $workflows = new GalaxyWorkflows($galaxy_instance);
  $gworkflow = $workflows->show(array('workflow_id' => $workflow->workflow_id));

  // If the UUIDs don't match then someone changed the workflow and it
  // is no longer guaranteed to be the same. So, we'll mark it as 'Altered'.
  if ($gworkflow['latest_workflow_uuid'] != $workflow->workflow_uuid) {
    db_update('tripal_galaxy_workflow')
      ->fields(array(
        'status' => 'Altered'
      ))
      ->condition('galaxy_workflow_id', $workflow->galaxy_workflow_id)
      ->execute();
  }

  return TRUE;
}

/**
 * Checks and updates the status of a Galaxy workflow.
 *
 * Each Galaxy workflow has a unique UUID.  Each time the workflow is changed
 * that UUID changes as well.
 */
function tripal_check_galaxy_submission_status($sid) {

  if (!$sid) {
    return FALSE;
  }
  if (!is_numeric($sid)) {
    return FALSE;
  }
  $workflow = db_select('tripal_galaxy_workflow_submission', 'tgws')
    ->fields('tgws')
    ->condition('tgws.sid', $sid)
    ->execute()
    ->fetchObject();

  // Connect to the Galaxy instance and get the list of workflows
  $galaxy_instance = tripal_get_galaxy_connection($workflow->galaxy_id);
  $workflows = new GalaxyWorkflows($galaxy_instance);
  $gworkflow = $workflows->show(array('workflow_id' => $workflow->workflow_id));



  return TRUE;
}