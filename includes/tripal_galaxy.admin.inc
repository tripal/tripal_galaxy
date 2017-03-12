<?php

/**
 * @file
 * This file contains the functions used for administration of the module
 */

/**
 * Admin home page for Tripal Galaxy
 *
 * @return Formatted HTML table containing information on all Galaxy Instances
 *
 */
function tripal_galaxy_admin_home() {

  // Initialize the headers and rows of the table.
  $rows = array();
  $headers = array('Galaxy Server', 'Description', 'Options');

  // Retrieve the list of galaxy servers that have been added and generate
  // the rows for the table.
  $sql = "SELECT * FROM {tripal_galaxy}";
  $results = db_query($sql);

  while ($result = $results->fetchObject()) {
    $rows[] = array($result->servername, $result->description,
      l('edit', '/admin/tripal/extension/galaxy/edit/' . $result->galaxy_id));
  }

  // Theme the table and return.
  $table = array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(
      'id' => 'tripal_galaxy-table-properties',
      'class' => 'tripal-data-table'
    ),
    'sticky' => FALSE,
    'caption' => '',
    'colgroups' => array(),
    'empty' => 'No Galaxy instances have been added.'
  );
  return theme_table($table);
}



/**
 * The themable function for the available workflow form
 *
 * @param $form
 * @param $form_state
 */
function theme_tripal_galaxy_admin_workflows_form_rows($variables) {
  $form_rows = $variables['form'];

  // The apparopriate headers for this form's table.
  $headers = array(
    'Enabled',
    'Workflow Name',
    'Galaxy Server',
    'Workflow ID',
    'Actions');

  // Theme the rows of this form's table.
  $rows = array();
  foreach (element_children($form_rows) as $i) {
    $row = $form_rows[$i];
    $rows[] = array(
      drupal_render($row["enabled-" . $i]),
      drupal_render($row["workflow_name-user" . $i]),
      drupal_render($row["servername-" . $i]),
      drupal_render($row["workflow_id-user" . $i]),
      drupal_render($row["actions-" . $i])
    );
  }

  // Subset sequences.
  return theme_table(array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(),
    'sticky' => TRUE,
    'caption' => '',
    'colgroups' => array(),
    'empty' => 'There are no workflows',
  ));
}

/**
 * Removes a workflow based on the given workflow id and node id
 *
 * @param $galaxy_workflow_id
 * @param $node_id
 */
function tripal_galaxy_admin_remove_workflow($galaxy_workflow_id, $node_id) {
  // TODO: Make sure this workflow has no jobs running
  // TODO: Make sure the galaxy_workflow_id is legit
  // TODO: Verify that the node

  // Delete the table instance given the galaxy_workflow_id
  $deleted = db_query("DELETE FROM {tripal_galaxy_workflow} WHERE galaxy_workflow_id = :wf_id",
      array(':wf_id' => $galaxy_workflow_id));
  $deleted = db_query("DELETE FROM {node} WHERE nid =:node", array(':node' => $node_id));
  if ($deleted == NULL || $deleted == FALSE) {
    drupal_set_message(t('Issues deleting the workflow'), 'error');
    return FALSE;
  }

  // Set message and go back to the workflow page.
  drupal_set_message(t('Workflow Deleted Successfully'));
  drupal_goto("/admin/tripal/extension/galaxy/workflows");
}
