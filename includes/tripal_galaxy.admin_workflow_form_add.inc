<?php

/**
 * Generates the form that lists the available workflows in the
 * default Galaxy instances and allows the site admin to select which ones
 * should be published on the site for end-users to use.
 */
function tripal_galaxy_admin_add_workflow_form($form, &$form_state) {
  $form['#tree'] = TRUE;

  // Grab the galaxy id from the form state and Obtain the default galaxy server
  // from the database.
  $galaxy_id = '';
  if (array_key_exists( 'values', $form_state )) {
    $galaxy_id = $form_state['values']['galaxy_id'];
  }
  $results = db_select( 'tripal_galaxy', 'tg' )
    ->fields('tg', array('galaxy_id', 'servername'))
    ->execute();
  $galaxies[0] = 'Select';
  while ($galaxy = $results->fetchObject()) {
    $galaxies[$galaxy->galaxy_id] = $galaxy->servername;
  }

  $i = 0;

  // Select which galaxy server to choose the workflows from
  $form['galaxy_id'] = array(
    '#type' => 'select',
    '#options' => $galaxies,
    '#title' => t ( 'Galaxy Servers' ),
    '#description' => t ( 'Please select a galaxy instance to view it\'s workflows' ),
    '#ajax' => array(
      'callback' => 'tripal_galaxy_admin_add_workflow_form_ajax_callback',
      'wrapper' => 'tripal-galaxy-admin-wf-available-form-rows',
      'method' => 'replace',
      'effect' => 'fade'
    ),
  );

  $form['#prefix'] = '<div id="tripal-galaxy-admin-wf-available-form-rows">';
  $form['#suffix'] = '</div>';

  // If a galaxy instance has been selected, Ajax-complete the form
  if ($galaxy_id) {

    // Galaxy id for the server
    $form['galaxy_id-server'] = array(
      '#type' => 'value',
      '#value' => $galaxy_id
    );

    // Connect to the Galaxy instance.
    $galaxy_instance = tripal_get_galaxy_connection($galaxy_id);


    // Check to make sure the connection is good.
    if ($galaxy_instance->getVersion() == NULL) {
      drupal_set_message('Cannot connect to Galaxy Instance', 'error');
      return;
    }

    // Obtain the workflows
    $workflows = new GalaxyWorkflows($galaxy_instance);
    $workflows = $workflows->index();

    // Loop through each workflow to add it to the form
    foreach ($workflows as &$workflow) {

      // First let's check to see if this workflow exists in our database,
      // if it does, assign default values as appropriate
      $workflow_exists = 1;
      $default_label = "";
      $default_description = "";
      $default_available = 0;
      $sql = "
        SELECT * FROM {tripal_galaxy_workflow}
        WHERE workflow_id = :workflow_id
        AND galaxy_id = :galaxy_id
      ";
      $workflow_instance = db_query($sql, array(
        ':workflow_id' => $workflow['id'],
        ':galaxy_id' => $galaxy_id
      ));
      $workflow_instance = $workflow_instance->fetchObject ();
      if ($workflow_instance == NULL || $workflow_instance == FALSE) {
        $workflow_exists = 0;
      }
      else {
        $default_label = $workflow_instance->label;
        $default_description = $workflow_instance->description;
      }

      // Workflow name
      $form['rows'][$i]['workflow_name-' . $i] = array(
        '#type' => 'item',
        '#markup' => $workflow['name']
      );
      $form['rows'][$i]['workflow_name-server-' . $i] = array(
        '#type' => 'value',
        '#value' => $workflow['name']
      );
      // Workflow_id
      $form['rows'][$i]['workflow_id-' . $i] = array(
        '#type' => 'item',
        '#markup' => $workflow['id']
      );
      // Workflow UUID
      $form['rows'][$i]['workflow_uuid-' . $i] = array(
        '#type' => 'value',
        '#value' => $workflow['latest_workflow_uuid']
      );
      $form['rows'][$i]['workflow_id-server-' . $i] = array(
        '#type' => 'value',
        '#value' => $workflow['id']
      );
      // Is added to site
      if ($workflow_exists == 1) {
        $form['rows'][$i]['available-' . $i] = array(
          '#type' => 'checkbox',
          '#default_value' => 1,
          '#disabled' => TRUE
        );
        $form['contains_wf-' . $i] = array(
          '#type' => 'value',
          '#value' => TRUE
        );
      }
      else {
        $form['rows'][$i]['available-' . $i] = array(
          '#type' => 'checkbox',
          '#default_value' => $workflow_exists
        );
        $form['contains_wf-' . $i] = array(
          '#type' => 'value',
          '#value' => FALSE
        );
      }
      $i++;
    }

    // The themable funciton for the form.
    $form['rows']['#theme'] = 'tripal_galaxy_admin_add_workflow_form_rows';

    // Default submit
    $form['submit'] = array(
      '#type' => 'submit',
      '#name' => 'submit',
      '#value' => t ( 'Submit' )
    );
  }

  return $form;
}

/**
 * Ajax Callback
 */
function tripal_galaxy_admin_add_workflow_form_ajax_callback(&$form, $form_state) {
  // Return the whole form.
  return $form;
}

/**
 * The themable fuinction for the add workflow form
 *
 */
function theme_tripal_galaxy_admin_add_workflow_form_rows($variables) {
  $form = $variables['form'];

  // The apparopriate headers for this form's table.
  $headers = array(
    'Add Workflow',
    'Workflow Name',
    'Workflow Id'
  );

  // Theme the rows of this form's table.
  $rows = array();
  $i = 0;
  foreach (element_children($form) as $i) {
    $row = $form[$i];
    $rows[] = array(
      drupal_render($row["available-" . $i]),
      drupal_render($row["workflow_name-" . $i]),
      drupal_render($row["workflow_id-" . $i]) . drupal_render($row["workflow_uuid-" . $i])
    );
    $i ++;
  }

  // Subset sequences.
  return theme_table(array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(),
    'sticky' => TRUE,
    'caption' => '',
    'colgroups' => array(),
    'empty' => 'No Workflows Available'
  ));
}

/**
 * TODO:
 * Override's Drupal's Hook_form_validate
 */
function tripal_galaxy_admin_add_workflow_form_validate($form, &$form_state) {
  // Not implemented yet
  return;
}

/**
 * Implements Drupal's Hook_form_submit
 * possible conditions:
 * - Workflow already exists on site and needs to be updated.
 * - Workflow does not exist on site and needs to be inserted.
 */
function tripal_galaxy_admin_add_workflow_form_submit($form, &$form_state) {
  $i = 0;
  $form_rows = $form_state['values']['rows'];

  // Go through all of the row entries.
  foreach ($form_rows as $i => $row) {

    // If this workflow is selected to be added to the site...
    if (($form_rows[$i]['available-' . $i] == 1) && ($form_state['values']['contains_wf-' . $i] == FALSE)) {

      // Insert the new workflow into the db given the data from the add wd form
      $transaction = db_transaction();
      try {
        $data = array(
          'workflow_name' => $form_rows[$i]['workflow_name-server-' . $i],
          'workflow_id' => $form_rows[$i]['workflow_id-server-' . $i],
          'galaxy_id' => $form_state['values']['galaxy_id-server'],
          'available' => $form_rows[$i]['available-' . $i],
          'workflow_uuid' =>  $form_rows[$i]['workflow_uuid-' . $i],
        );

        // Create the appropriate node for this workflow
        module_load_include('inc', 'tripal_galaxy', 'includes/tripal_galaxy.webform');
        $nid = tripal_galaxy_build_webform($data['galaxy_id'], $data['workflow_id']);
        if (!$nid) {
          throw new Exception('Could not create workflow');
        }

        $data['nid'] = $nid;
        $workflow = drupal_write_record('tripal_galaxy_workflow', $data);

        $instructions = array(
          'the workflow title.',
          'the instructions for the workflow.',
          'the "About this step" field for each tool to help the user understand the workflow.',
          'each tool setting descriptions as desired.',
        );
        $list = theme_item_list(array(
          'items' => $instructions,
          'title' => '',
          'type' => 'ul',
          'attributes' => array(),
        ));
        drupal_set_message(t('The workflow was succesfully added! <p>Sometimes
          these workflows need a bit of tweaking to make them more intuitive
          for users. Click the title of the workflow below to customize how it
          will appear to users. Consider improving: !list</p>', array('!list' => $list)));
      }
      catch (Exception $e) {
        $transaction->rollback();
        drupal_set_message($e->getMessage(), 'error');
        //watchdog_exception('tripal_galaxy', $e);
      }
    }
    $i ++;
  }

  // Return to the previous page
  drupal_goto("/admin/tripal/extension/galaxy/workflows");
  return;
}
