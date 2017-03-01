<?php

/**
 * Implements Drupal's hook_form function, responsible for displaying all of the
 * available Workflows
 */
function tripal_galaxy_admin_workflows_form($form, &$form_state) {

  // TODO: This is not keeping track of what workflow comes from which
  // server
  $workflow_instances = db_query("SELECT * FROM {tripal_galaxy_wf}");
  $i = 0;
  $form['#tree'] = TRUE;

  // Loop through each workflow to add it to the form
  while ($workflow = $workflow_instances->fetchObject()) {

    // Obtain the corresponding Galaxy instance for the workflow
    $galaxy_instance = db_query('SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id',
        array('galaxy_id' => $workflow->galaxy_id));
    $galaxy_instance = $galaxy_instance->fetchObject();

    // The galaxy id
    $form['rows'][$i]['galaxy-' . $i] = array(
      '#type' => 'value',
      '#value' => $galaxy_instance
    );

    $form['rows'][$i]['enabled-' . $i] = array(
      '#type' => 'checkbox',
      '#default_value' => $workflow->status
    );

    // Workflow name
    $form['rows'][$i]['workflow_name-' . $i] = array(
      '#type' => 'value',
      // TODO: I need to keep track of the proper node that was created when
      // this workflow was added
      '#value' => $workflow->workflow_name,
    );

    $form['rows'][$i]['workflow_name-user' . $i] = array(
      '#type' => 'item',
      '#markup' => l($workflow->workflow_name, '/node/' . $workflow->node),
    );

    // Servername that the workflow's on
    $form['rows'][$i]['servername-' . $i] = array(
      '#type' => 'item',
      '#markup' => $galaxy_instance->servername,
    );

    // Workflow_id for user and or internal:
    $form['rows'][$i]['workflow_id-user' . $i] = array(
      '#type' => 'item',
      '#markup' => $workflow->workflow_id
    );
    $form['rows'][$i]['workflow_id-' . $i] = array(
      '#type' => 'value',
      '#value' => $workflow->workflow_id
    );

    // Remove link
    // TODO: I need a confirm/validate here as seen on any generic content
    // delete
    $form['rows'][$i]['actions-' . $i] = array(
      '#type' => 'item',
      '#markup' => l('Remove', '/admin/tripal/extension/galaxy/workflows/remove/' . $workflow->galaxy_wf_id)
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
    $sql = "SELECT * FROM {tripal_galaxy_wf} WHERE workflow_id = :workflow_id AND galaxy_id = :galaxy_id";
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
        'galaxy_wf_id' => $workflow_instance->galaxy_wf_id,
        'workflow_name' => $form_rows[$i]['workflow_name-' . $i],
      );
      $workflow = drupal_write_record ( 'tripal_galaxy_wf', $data, 'galaxy_wf_id' );
      drupal_set_message ( t ( 'Workflow information Updated' ) );
    }
    $i ++;
  }
}
