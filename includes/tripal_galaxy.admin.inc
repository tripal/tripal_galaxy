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
  $headers = array('Galaxy Server', 'Description', 'Status', 'Options');

  // Retrieve the list of galaxy servers that have been added and generate
  // the rows for the table.
  $sql = "SELECT * FROM {tripal_galaxy}";
  $results = db_query($sql);

  while ($result = $results->fetchObject()) {

    $disabled_workflows = db_select('tripal_galaxy_workflow', 'tgw')
      ->fields('tgw')
      ->condition('galaxy_id', $result->galaxy_id)
      ->condition('status', 'disabled')
      ->execute()
      ->fetchObject();

    $enabled_workflows = db_select('tripal_galaxy_workflow', 'tgw')
      ->fields('tgw')
      ->condition('galaxy_id', $result->galaxy_id)
      ->condition('status', 'Active')
      ->execute()
      ->fetchObject();

    //Check the status of the server to determine which link to include.
    $status_link = '';
    if ($result->serverstatus == 'unavailable') {
      if ($disabled_workflows && $enabled_workflows == NULL) {
        $status_link = l("enable all workflows", '/admin/tripal/extension/galaxy/enable/' . $result->galaxy_id);
      }
      else {
        $status_link = l("disable all workflows", '/admin/tripal/extension/galaxy/disable/' . $result->galaxy_id);
      }
    }
    elseif ($result->serverstatus == 'available') {
        if ($disabled_workflows) {
          $status_link = l("\t enable all workflows", '/admin/tripal/extension/galaxy/enable/' . $result->galaxy_id);
        }
        else {
          $status_link = l("\t disable all workflows", '/admin/tripal/extension/galaxy/disable/' . $result->galaxy_id);
        }

    }
    $rows[] = array(
      $result->servername,
      $result->description,
      $result->serverstatus,
      l('edit', '/admin/tripal/extension/galaxy/edit/' . $result->galaxy_id) . ' | ' .
      l('test', '/admin/tripal/extension/galaxy/test/' . $result->galaxy_id) . ' | ' . 
      $status_link
    );
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
 * Removes a workflow based on the given workflow id.
 *
 * This function is called by Drupal's confirm_form which is setup in the
 * tripal_galaxy_admin_confirm_remove_workflow_form() function.
 */
function tripal_galaxy_admin_remove_workflow($form, &$form_state) {

  $transaction = db_transaction();

  try {

    // Get the Node that belongs to this workflow
    $workflow = $form['#workflow'];

    if (!$workflow) {
      drupal_set_message(t('Cannot remove the workflow, as it cannot be found'), 'error');
      drupal_goto("/admin/tripal/extension/galaxy/workflows");
      return;
    }

    // Remove the webform node that this workflow belongs to.
    node_delete($workflow->nid);

    // Delete the table instance given the galaxy_workflow_id
    $success = db_delete('tripal_galaxy_workflow')
      ->condition('galaxy_workflow_id', $workflow->galaxy_workflow_id)
      ->execute();
    $success = db_delete('tripal_galaxy_workflow_submission')
      ->condition('galaxy_workflow_id', $workflow->galaxy_workflow_id)
      ->execute();
  }
  catch (Exception $e) {
    $transaction->rollback();
    drupal_set_message($e->getMessage(), 'error');
    drupal_set_message(t('Cannot remove the workflow'), 'error');
    drupal_goto("/admin/tripal/extension/galaxy/workflows");
    return;
  }

  // Set message and go back to the workflow page.
  drupal_set_message(t('Workflow Deleted Successfully'));
  drupal_goto("/admin/tripal/extension/galaxy/workflows");
}


/**
 * Form callback: confirmation form for deleting a tripal_entity.
 *
 * @param $tripal_entity The
 *          tripal_entity to delete
 *
 * @see confirm_form()
 */
function tripal_galaxy_admin_confirm_remove_workflow_form($form, &$form_state, $galaxy_workflow_id) {
  // Get the Node that belongs to this workflow
  $workflow = db_select('tripal_galaxy_workflow', 'tgw')
    ->fields('tgw')
    ->condition('galaxy_workflow_id', $galaxy_workflow_id)
    ->execute()
    ->fetchObject();

  $form['#workflow'] = $workflow;
  $form['#submit'][] = 'tripal_galaxy_admin_remove_workflow';

  $form = confirm_form(
    // Form.
    $form,
    // Question.
    t('Confirm deletion of the workflow titled: %title', array('%title' => $workflow->workflow_name)),
    // The page to go to if the user cancels the action.
    '/admin/tripal/extension/galaxy/workflows',
    // Description.
    '<p>' . t('This action cannot be undone and all job submissions for this workflow will be lost.') .'</p>',
    // Yes text.
    t('Delete'),
    // No text.
    t('Cancel'),
    // Internal name for this confirm_form.
    'confirm'
  );

  return $form;
}

/**
 * Responds to the menu item admin/tripal/extension/galaxy/test/%.
 *
 * @param  $galaxy_id
 *   A galaxy server ID
 */
function tripal_galaxy_admin_test_server($galaxy_id) {
  $status = tripal_galaxy_test_connection(array('galaxy_id' => $galaxy_id));
  if ($status === FALSE) {
    // Update status in the db.
    db_update('tripal_galaxy')
      ->fields(array(
        'serverstatus' => 'unavailable',
      ))
      ->condition('galaxy_id', $galaxy_id, '=')
      ->execute();
  }
  else if ($status === TRUE) {
    // Update status in the db.
    db_update('tripal_galaxy')
      ->fields(array(
        'serverstatus' => 'available',
      ))
      ->condition('galaxy_id', $galaxy_id, '=')
      ->execute();
  }
  drupal_goto('admin/tripal/extension/galaxy');
}

/**
 * Responds to the menu item admin/tripal/extension/galaxy/disable/%.
 *
 * @param  $galaxy_id
 *   A galaxy server ID
 */
function tripal_galaxy_admin_disable_workflows($galaxy_id) {

  try {
    $sql = "SELECT * FROM {tripal_galaxy_workflow} WHERE galaxy_id = $galaxy_id";
    $results = db_query($sql);

    while ($result = $results->fetchObject()) {
      $sucess = db_update('tripal_galaxy_workflow')
        ->fields(array(
          'status' => 'disabled',
        ))
        ->condition('galaxy_id', $galaxy_id, '=')
        ->execute();
    }
    drupal_set_message(t('All workflows disabled'), 'status');
    drupal_goto("/admin/tripal/extension/galaxy");

  } catch (Exception $e) {
    $transaction->rollback();
    drupal_set_message($e->getMessage(), 'error');
    drupal_set_message(t('Cannot disable the workflows'), 'error');
    drupal_goto("/admin/tripal/extension/galaxy");
    return;
  }
}

/**
 * Responds to the menu item admin/tripal/extension/galaxy/disable/%.
 *
 * @param  $galaxy_id
 *   A galaxy server ID
 */
function tripal_galaxy_admin_disable_workflow($galaxy_workflow_id)
{

  try {
    db_update('tripal_galaxy_workflow')
      ->fields(array(
        'status' => 'disabled',
      ))
      ->condition('galaxy_workflow_id', $galaxy_workflow_id, '=')
      ->execute();
    
    drupal_set_message(t('Workflow disabled'), 'status');
    drupal_goto("/admin/tripal/extension/galaxy/workflows");

  } catch (Exception $e) {
    $transaction->rollback();
    drupal_set_message($e->getMessage(), 'error');
    drupal_set_message(t('Cannot disable the workflow'), 'error');
    drupal_goto("/admin/tripal/extension/galaxy/workflows");
    return;
  }
}

/**
 * Responds to the menu item admin/tripal/extension/enable/test/%.
 *
 * @param  $galaxy_id
 *   A galaxy server ID
 */
function tripal_galaxy_admin_enable_workflows($galaxy_id) {
  try {
    $sql = "SELECT * FROM {tripal_galaxy_workflow} WHERE galaxy_id = $galaxy_id";
    $results = db_query($sql);

    while ($result = $results->fetchObject()) {
      $sucess = db_update('tripal_galaxy_workflow')
        ->fields(array(
          'status' => 'Active',
        ))
        ->condition('galaxy_id', $galaxy_id, '=')
        ->execute();
    }
    drupal_set_message(t('All workflows enabled'), 'status');
    drupal_goto("/admin/tripal/extension/galaxy");

  } catch (Exception $e) {
    $transaction->rollback();
    drupal_set_message($e->getMessage(), 'error');
    drupal_set_message(t('Cannot disable the workflows'), 'error');
    drupal_goto("/admin/tripal/extension/galaxy");
    return;
  }
}

/**
 * Responds to the menu item admin/tripal/extension/enable/test/%.
 *
 * @param  $galaxy_id
 *   A galaxy server ID
 */
function tripal_galaxy_admin_enable_workflow($galaxy_workflow_id)
{
  try {
    $sucess = db_update('tripal_galaxy_workflow')
      ->fields(array(
        'status' => 'Active',
      ))
      ->condition('galaxy_workflow_id', $galaxy_workflow_id, '=')
      ->execute();
    
    drupal_set_message(t('Workflow enabled'), 'status');
    drupal_goto("/admin/tripal/extension/galaxy/workflows");

  } catch (Exception $e) {
    $transaction->rollback();
    drupal_set_message($e->getMessage(), 'error');
    drupal_set_message(t('Cannot enable the workflow'), 'error');
    drupal_goto("/admin/tripal/extension/galaxy/workflows");
    return;
  }
}
