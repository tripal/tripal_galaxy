<?php

/**
 * Implements Drupal's hook_form function, responsible for displaying all of the
 * available Workflows
 */
function tripal_galaxy_admin_workflows_form($form, &$form_state) {

  $form['#tree'] = TRUE;

  // Get the list of workflows and interate through them.
  $workflows = db_select('tripal_galaxy_workflow', 'tgw')
    ->fields('tgw')
    ->orderBy('workflow_name')
    ->execute();
  $i = 0;
  while ($workflow = $workflows->fetchObject()) {
    $galaxy = db_select('tripal_galaxy', 'tg')
      ->fields('tg')
      ->condition('galaxy_id', $workflow->galaxy_id)
      ->execute()
      ->fetchObject();

    $node = node_load($workflow->nid);

    $form['rows'][$i]['enabled-' . $i] = array(
      '#type' => 'checkbox',
      '#default_value' => $workflow->status
    );
    $form['rows'][$i]['galaxy_workflow_id' . $i] = array(
      '#type' => 'item',
      '#markup' => $workflow->galaxy_workflow_id,
    );
    $form['rows'][$i]['workflow_name' . $i] = array(
      '#type' => 'item',
      '#markup' => l($node->title, 'node/' . $workflow->nid),
    );
    $form['rows'][$i]['workflow_name_galaxy' . $i] = array(
      '#type' => 'item',
      '#markup' => $workflow->workflow_name,
    );
    $form['rows'][$i]['servername-' . $i] = array(
      '#type' => 'item',
      '#markup' => $galaxy->servername,
    );
    $form['rows'][$i]['workflow_status-' . $i] = array(
      '#type' => 'item',
      '#markup' => $workflow->status
    );
    $form['rows'][$i]['workflow_ids-' . $i] = array(
      '#type' => 'item',
      '#markup' => 'Workflow ID: ' . $workflow->workflow_id . '</br>' .
        'Workflow UUID: ' . $workflow->workflow_uuid . '',
    );
    if ($workflow->status == 'Active') {
      $status_change = l("Disable", '/admin/tripal/extension/galaxy/workflows/disable/' . $workflow->galaxy_workflow_id);
    } else if ($workflow->status == 'disabled') {
        $status_change = l("Enable", '/admin/tripal/extension/galaxy/workflows/enable/' . $workflow->galaxy_workflow_id);
    }
    $form['rows'][$i]['actions-' . $i] = array(
      '#type' => 'item',
      '#markup' => l('Remove', '/admin/tripal/extension/galaxy/workflows/remove/' . $workflow->galaxy_workflow_id) . '  |  ' .
      $status_change
    );
   
    $i++;
  }

  // Theme this form
  $form['rows']['#theme'] = 'tripal_galaxy_admin_workflows_form_rows';

  // Default submit
  $form['submit'] = array(
    '#type' => 'submit',
    '#name' => 'submit',
    '#value' => t('Submit Changes')
  );

  return $form;
}

/**
 * The themable fuinction for the available workflow form
 *
 * @param $form
 * @param $form_state
 */
function theme_tripal_galaxy_admin_workflows_form_rows($variables) {
  $form_rows = $variables['form'];

  // The apparopriate headers for this form's table.
  $headers = array(
    'ID',
    'Workflow Name',
    'Galaxy Server',
    'Status',
    //'Galaxy IDs',
    'Actions'
  );

  // Theme the rows of this form's table.
  $rows = array();
  foreach (element_children($form_rows) as $i) {
    $row = $form_rows[$i];
    $rows[] = array(
      drupal_render($row["galaxy_workflow_id" . $i]),
      '<i>Name on this site:</i> ' . drupal_render($row["workflow_name" . $i]) .
      '<i>Name on Galaxy server:</i> ' . drupal_render($row["workflow_name_galaxy" . $i]),
      drupal_render($row["servername-" . $i]),
      drupal_render($row["workflow_status-" . $i]),
     // drupal_render($row["workflow_ids-" . $i]),
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
 * Override's Drupal's Hook_form_submit for the available workflow form
 *
 * @param $form
 * @param $form_state
 */
function tripal_galaxy_admin_workflows_form_submit($form, &$form_state) {
  $i = 0;
  $form_rows = $form_state['values']['rows'];

  // Go through all of the row entries.
  foreach ($form_rows as $i => $row) {

      // Select the workflow being edited
    $sql = "SELECT * FROM {tripal_galaxy_workflow} WHERE workflow_id = :workflow_id AND galaxy_id = :galaxy_id";
    $galaxy = $form_rows[$i]['galaxy-' . $i];
    $galaxy_id = $galaxy->galaxy_id;
    $args = array (
      ':workflow_id' => $form_rows[$i]['workflow_id-' . $i] ,
      ':galaxy_id' => $galaxy_id,
    );
    $workflow_instance = db_query ($sql, $args);
    $workflow_instance = $workflow_instance->fetchObject ();

    // If the workflow has changed values, update it
    if ($workflow_instance->status != $form_rows[$i]['enabled-' . $i]) {
      $data = array (
        'status' => $form_rows[$i]['enabled-' . $i],
        'galaxy_workflow_id' => $workflow_instance->galaxy_workflow_id,
        'workflow_name' => $form_rows[$i]['workflow_name-' . $i],
      );
      $workflow = drupal_write_record('tripal_galaxy_workflow', $data, 'galaxy_workflow_id' );
      drupal_set_message ( t ( 'Workflow information Updated' ) );
    }
    $i ++;
  }
}
