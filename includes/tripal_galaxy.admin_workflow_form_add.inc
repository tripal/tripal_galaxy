<?php

/**
 * Generates the form that lists the available workflows in the
 * default Galaxy instances and allows the site admin to select which ones
 * should be published on the site for end-users to use.
 *
 * @param
 *          $form
 * @param
 *          $form_state
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
    ->fields('tg', array('galaxy_id', 'servername', 'label'))
    ->execute();
  $galaxies[0] = 'Select';
  while ($galaxy = $results->fetchObject()) {
    $galaxies[$galaxy->galaxy_id] = $galaxy->servername;
  }
  //dpm($galaxies);
  //dpm($galaxy_id);

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
    '#default_value' => 2,
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
    // Grab the appropriate galaxy server table entry
    $sql = "SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id";
    $galaxy_server = db_query($sql, array(':galaxy_id' => $galaxy_id));
    $galaxy_server = $galaxy_server->fetchObject();

    // Obtain the hostname and port from the url
    $matches = array();
    $use_https = FALSE;
    $http_regex = "((https|http):\/\/)?";
    $host_regex = "([a-z0-9-.]*)";
    $port_regex = "(:)?([0-9]*)?";
    $regex = $http_regex . $host_regex . $port_regex;
    preg_match("/^$regex$/", $galaxy_server->url, $matches);
    $hostname = $matches[3];
    $port = $matches[5];

    // dpm($galaxy_server->url);
    if($matches[2] == "https")
      $use_https = TRUE;

    // dpm($port);
    // dpm($matches);
    // dpm($use_https);

    // Create a new galaxy instance using the obtained hostname and port
    // Then Authenticate
    libraries_load('blend4php');
    $galaxy_instance = new GalaxyInstance($hostname, $port, $use_https);

    $galaxy_instance->setAPIKey($galaxy_server->api_key);


    // Check to make sure the connection is good.
    if ($galaxy_instance->getVersion() == NULL)
      drupal_set_message('Cannot connect to Galaxy Instance', 'error');


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
      $workflow_instance = db_query ( $sql, array(
        ':workflow_id' => $workflow['id'],
        ':galaxy_id' => $galaxy_id
      ) );
      $workflow_instance = $workflow_instance->fetchObject ();
      if ($workflow_instance == NULL || $workflow_instance == FALSE)
        $workflow_exists = 0;
      else
      {
        $default_label = $workflow_instance->label;
        $default_description = $workflow_instance->description;
      }

      // Workflow name
      $form['rows'][$i]['workflow_name-' . $i] = array(
        '#type' => 'item',
        '#markup' => $workflow['name']
      )
      ;
      $form['rows'][$i]['workflow_name-server-' . $i] = array(
        '#type' => 'value',
        '#value' => $workflow['name']
      );

      // Workflow_id
      $form['rows'][$i]['workflow_id-' . $i] = array(
        '#type' => 'item',
        '#markup' => $workflow['id']
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
      drupal_render($row["workflow_id-" . $i])
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
          'available' => $form_rows[$i]['available-' . $i]
        );

        // Create the appropriate node for this workflow
        module_load_include('inc', 'tripal_galaxy', 'includes/tripal_galaxy.webform');
        $nid = tripal_galaxy_build_webform($data['galaxy_id'], $data['workflow_id']);
        if (!$nid) {
          throw new Exception('Could not create workflow');
        }

        $data['node'] = $nid;
        $workflow = drupal_write_record('tripal_galaxy_workflow', $data);

        drupal_set_message(t('Workflow Added'));
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
