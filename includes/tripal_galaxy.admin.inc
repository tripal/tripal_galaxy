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
 * Implement's Drupal's hook_form
 *
 * @param $form
 * @param $form_state
 * @param $galaxy_id
 *
 * @return $form
 */
function tripal_galaxy_admin_configuration_form($form, $form_state, $galaxy_id = NULL) {
  $default_server_name = "";
  $default_user_name = "";
  $default_description = "";
  $default_api_key = "";
  $default_url = "";

  // If a galaxy id is set, feel free to declare all the default values with the
  // appropriate
  // Galaxy instance
  if ($galaxy_id != NULL) {
    $sql = "SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id";
    $galaxy_instance = db_query($sql, array(':galaxy_id' => $galaxy_id));
    $galaxy_instance = $galaxy_instance->fetchObject();
    if ($galaxy_instance != NULL) {
      $default_api_key = $galaxy_instance->api_key;
      $default_description = $galaxy_instance->description;
      $default_server_name = $galaxy_instance->servername;
      $default_user_name = $galaxy_instance->username;
      $default_url = $galaxy_instance->url;
    }
  }

  // The galaxy id
  $form['galaxy_id'] = array(
    '#type' => 'value',
    '#value' => $galaxy_id
  );

  // Server Name
  $form['servername'] = array(
    '#title' => t('Galaxy Server Name'),
    '#type' => 'textfield', '#size' => 50,
    '#default_value' => $default_server_name,
    '#description' => 'Please provide the name of the remote Galaxy Server',
    '#required' => TRUE
  );

  // Description
  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#default_value' => $default_description,
    '#description' => 'Provide any additional details about this server.',
    '#required' => FALSE
  );

  // URL
  $form['url'] = array(
    '#title' => t('URL'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value' => $default_url,
    '#description' => 'The URL for the remote Galaxy server.',
    '#required' => TRUE
  );

  // User Name
  $form['username'] = array('#title' => t('User Name'), '#type' => 'textfield', '#size' => 50,
    '#default_value' => $default_user_name,
    '#description' => 'The user name for the Galaxy server.
        This username is used to launch all jobs by default.
        If this field is left blank then it is expected that the user
        has an account on the Galaxy server and will provide their username
        when executing workflows.', '#required' => FALSE);

  // API Key
  $form['api_key'] = array('#title' => t('API Key'), '#type' => 'textfield', '#size' => 50,
    '#default_value' => $default_api_key,
    '#description' => "The API key for the user name specified above.
        If this field is left blank then it is expected that the user
        will provide their own API key when submitting a job.", '#required' => FALSE);

  // Default submit
  $form['submit'] = array(
    '#type' => 'submit',
    '#name' =>
    'submit',
    '#value' => t('Submit')
  );

  // Test Connection
  $form['test_connect'] = array(
    '#type' => 'submit',
    '#name' => 'test_connect',
    '#value' => t('Test Connection')
  );

  // Delete
  if ($galaxy_id) {
    $form['delete'] = array(
      '#type' => 'submit',
      '#name' => 'delete',
      '#value' => t('Delete')
    );
  }

  return $form;
}

/**
 * Custom Validate for hook_form
 *
 * @param
 *          $form
 * @param
 *          $form_state
 * @return boolean Return true if the form is valid for deletion, false
 *         otherwise
 */
function tripal_galaxy_admin_configuration_form_delete_validate($form, &$form_state) {

  // Check to see if we are deleting an instance.
  if ($form_state['clicked_button']['#name'] == 'delete') {
    $sql = "SELECT * FROM {tripal_galaxy} WHERE servername=" . $form['tripal_galaxy_server_name']['#value'];
    $results = db_result(db_query($sql));
    if ($results == NULL || $results == FALSE) {

      // If there is no entry with this servername return False.
      form_set_error('servername', t('A galaxy instance with that servername does not exist'));
      return FALSE;
    }
  }
}

/**
 * Implements Drupal's hook_form_submit
 * Handles submitting a new galaxy instance to the Galaxy table
 *
 * @param
 *          $form
 * @param
 *          $form_state
 */
function tripal_galaxy_admin_configuration_form_submit($form, &$form_state) {

   // Initialize the data to be sent to the tripal_galaxy table
  $data = array('servername' => $form_state['values']['servername'],
    'description' => $form_state['values']['description'],
  	'url' => $form_state['values']['url'],
    'username' => $form_state['values']['username'],
  	'api_key' => $form_state['values']['api_key']);

  // If we are deleting the given instance, go through the appropriate
  // motions...
  if ($form_state['clicked_button']['#name'] == 'delete') {
    $galaxy_instance = db_query("DELETE FROM {tripal_galaxy} WHERE servername = :servername",
        array(':servername' => $form_state['values']['servername']));

    // Set error if appripriate
    if ($galaxy_instance === FALSE || $galaxy_instance == NULL) {
      drupal_set_message(t('Error deleting record'), 'error');
      return;
    }
    else {
      // If there is no error, redirect.
      drupal_set_message(t('Galaxy instance has been deleted'));
      drupal_goto("/admin/tripal/extension/galaxy");
    }
  }
  // If we are not deleting, are we submitting?
  else if ($form_state['clicked_button']['#name'] == 'submit') {
    // If there is a galaxy ID this will be an edit.
    if ($form_state['values']['galaxy_id']) {
      $data['galaxy_id'] = $form_state['values']['galaxy_id'];
      $galaxy_instance = drupal_write_record('tripal_galaxy', $data, 'galaxy_id');
    } // Otherwise it's an insert.
    else {
      $galaxy_instance = drupal_write_record('tripal_galaxy', $data);
    }

    // Insert the new Galaxy Instance into Drupal, return where appropriate.
    if (!$galaxy_instance) {
      drupal_set_message(t('Problem inserting galaxy instance'), 'error');
      return;
    }

    // Default message.
    drupal_set_message(t('Galaxy instance has been recorded' . var_dump($galaxy_instance)));
    drupal_goto("/admin/tripal/extension/galaxy");
  }

  // Else if we are testing the connection.
  else {
    $form_state['rebuild'] = TRUE;

    // Obtain the hostname and port from the url
    $matches = array();
    $use_https = FALSE;
    $http_regex = "((https|http):\/\/)?";
    // TODO: Need to have variability here
    // $host_regex = "([a-z0-9-.]*).([a-z0-9-.]*)";
    $host_regex = "([a-z0-9-.]*)";
    $port_regex = "(:)?([0-9]*)?";
    $regex = $http_regex . $host_regex . $port_regex;
    preg_match("/^$regex$/", $data['url'], $matches);
    $hostname = $matches[3];
    $port = $matches[5];

    if($matches[2] == "https")
      $use_https = TRUE;

    // Create a new galaxy instance using the obtained hostname and port
    // Then Authenticate
    $library = libraries_load('blend4php');
    $galaxy_instance = new GalaxyInstance($hostname, $port, $use_https);
    $galaxy_instance->setAPIKey($data['api_key']);

    // Tell the user whether the connection was successful based on
    // getVersion()
    if ($galaxy_instance->getVersion() == NULL) {
      drupal_set_message(t('No Connection'), 'error');
    }
    else {
      drupal_set_message(t('Connection successful'));
    }
  }
}



/**
 * Implements Drupal's hook_form function, responsible for displaying all of the
 * available
 * Workflows
 */
function tripal_galaxy_admin_workflows_form($form, &$form_state) {
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
      '#value' => $workflow->workflow_name
    );
    $form['rows'][$i]['workflow_name-user' . $i] = array(
      '#type' => 'item',
      '#markup' => $workflow->workflow_name
    );

    // Servername that the workflow's on
    $form['rows'][$i]['servername-' . $i] = array(
      '#type' => 'item',
      '#markup' => $galaxy_instance->servername
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
    $form['rows'][$i]['remove-' . $i] = array(
      '#type' => 'item',
      '#markup' => l('Remove', '/admin/tripal/extension/galaxy/workflows/remove/' . $workflow->galaxy_wf_id)
    );

    // Description (Editable)
    $form['rows'][$i]['description-' . $i] = array(
      '#type' => 'textarea',
      '#rows' => 1,
      '#cols' => 20,
      '#default_value' => $workflow->description
    );

    // Workflow_label (Editable)
    $form['rows'][$i]['label-' . $i] = array(
      '#type' => 'textfield',
      '#size' => 25,
      '#default_value' => $workflow->label
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
    'Description',
    'Onsite Label',
    'Galaxy Server',
    'Workflow ID',

  'Remove');

  // Theme the rows of this form's table.
  $rows = array();
  foreach (element_children($form_rows) as $i) {
    $row = $form_rows[$i];
    $rows[] = array(
      drupal_render($row["enabled-" . $i]),
      drupal_render($row["workflow_name-user" . $i]),
      drupal_render($row["description-" . $i]),
      drupal_render($row["label-" . $i]),
      drupal_render($row["servername-" . $i]),
      drupal_render($row["workflow_id-user" . $i]),
      drupal_render($row["remove-" . $i])
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
    'empty' => 'No Workflows On Record',
  ));
}

/**
 * Override's Drupal's Hook_form_validate for the workflows form
 * Assures there are no duplicate labels entered for the workflows
 *
 * @param $form
 * @param $form_state
 */
function tripal_galaxy_admin_workflows_form_validate($form, &$form_state) {
  $i = 0;
  $form_rows = $form_state['values']['rows'];

  // Keeps track of all labels used.
  $labels = array();

  // Go through all of the row entries.
  foreach ($form_rows as $i => $row) {
    $workflow_id = $form_rows[$i]['workflow_id-' . $i];
    $label = $form_rows[$i]['label-' . $i];

    // Select the current workflow record.
    $sql = "SELECT * FROM {tripal_galaxy_wf} WHERE workflow_id = :wf_id";
    $workflow_instance = db_query($sql, array(':wf_id' => $workflow_id));
    $workflow_instance = $workflow_instance->fetchObject();

    // Make sure the label isn't already being used by a different workflow
    // other than this one.
    if ($workflow_instance->label != $label) {
      $sql = "
        SELECT galaxy_wf_id
        FROM {tripal_galaxy_wf}
        WHERE label = :label and NOT workflow_id = :wf_id
        LIMIT 1 OFFSET 0
      ";
      $args = array(':wf_id' => $workflow_id, ':label' => $label);
      $has_match = db_query($sql, $args)->fetchField();
      if ($has_match != FALSE && $has_match != NULL) {
        form_set_error("rows][$i][label-$i", t('A workflow with the label, "' . $label . '", already exists.'));
      }
      return FALSE;
    }

    // Make sure labels aren't bein reused.
    if (array_key_exists($label, $labels)) {
      form_set_error("rows][$i][label-$i", t('The label, "'. $label . '", is used more than once.'));
    }
    $labels[$label] = TRUE;
  }

  return;
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
    $workflow_instance = db_query ( $sql, $args );
    $workflow_instance = $workflow_instance->fetchObject ();

    // If the workflow has changed values, update it
    if ($workflow_instance->label != $form_rows[$i]['label-' . $i]
        || $workflow_instance->description != $form_rows[$i]['description-' . $i]) {
      $data = array (
        'status' => $form_rows[$i]['enabled-' . $i],
        'galaxy_wf_id' => $workflow_instance->galaxy_wf_id,
        'workflow_name' => $form_rows[$i]['workflow_name-' . $i],
        'label' => $form_rows[$i]['label-' . $i],
        'description' => $form_rows[$i]['description-' . $i]
      );
      $workflow = drupal_write_record ( 'tripal_galaxy_wf', $data, 'galaxy_wf_id' );
      drupal_set_message ( t ( 'Workflow information Updated' ) );
    }
    $i ++;
  }
}

/**
 * Removes a workflow based on the given galaxy id and workflow id
 *
 * @param $galaxy_id
 * @param $workflow_id
 */
function tripal_galaxy_admin_remove_wf($galaxy_wf_id) {
  // TODO: Make sure this workflow has no jobs running
  // TODO: Make sure the galaxy_wf_id is legit

  // Delete the table instance given the galaxy_wf_id
  $deleted = db_query("DELETE FROM {tripal_galaxy_wf} WHERE galaxy_wf_id = :wf_id",
      array(':wf_id' => $galaxy_wf_id));
  if ($deleted == NULL || $deleted == FALSE) {
    drupal_set_message(t('Issues deleting the workflow'), 'error');
    return FALSE;
  }

  // Set message and go back to the workflow page.
  drupal_set_message(t('Workflow Deleted Successfully'));
  drupal_goto("/admin/tripal/extension/galaxy/workflows");
}

// TODO: Generates a table that provides a listing of the Galaxy workflow usage
// statistics
// This is a form because it should provide filters (i.e to filter rows by user,
// etc.).
function tripal_galaxy_admin_usage_form() {

}

// TODO: Validates the tripal galaxy usage from.
function tripal_galaxy_admin_usage_form_validate() {
}

// TODO: Submits the admin usage form.
function tripal_galaxy_admin_usage_form_submit() {
}



function tripal_galaxy_admin_file_listing() {
  $output = '';

  // set the breadcrumb
  $breadcrumb = array();
  $breadcrumb[] = l('Home', '<front>');
  $breadcrumb[] = l('Administration', 'admin');
  $breadcrumb[] = l('Tripal', 'admin/tripal');
  $breadcrumb[] = l('Chado', 'admin/tripal/chado');
  $breadcrumb[] = l('galaxy', 'admin/tripal/chado/tripal_galaxy');
  drupal_set_breadcrumb($breadcrumb);

  // EXPLANATION: Typically for all Tripal modules the home administrative
  // page for the module contains a search form to help the adminstrator
  // locate records. The following galaxy code adds a default View to
  // the page
  /*
   * // Add the view
   * $view = views_embed_view('tripal_galaxy_admin_galaxy','default');
   * if (isset($view)) {
   * $output .= $view;
   * }
   * else {
   * $output .= '<p>The Tripal galaxy Module uses primarily views to provide an
   * '
   * . 'administrative interface. Currently one or more views needed for this '
   * . 'administrative interface are disabled. <strong>Click each of the
   * following links to '
   * . 'enable the pertinent views</strong>:</p>';
   * $output .= '<ul>';
   * $output .= '<li>'.l('galaxy Admin',
   * 'admin/tripal/chado/tripal_galaxy/views/galaxy/enable').'</li>';
   * $output .= '</ul>';
   * }
   */
  $output = 'Typically a search view goes here';

  return $output;
}

/**
 * Administrative settings form
 *
 * @ingroup tripal_galaxy
 */
function tripal_galaxy_admin() {
  $form = array();

  $form['nothing'] = array('#markup' => t('There are currently no settings to configure.'));

  // If your module is using the Chado Node: Title & Path API to allow custom
  // titles
  // for your node type then you need to add the configuration form for this
  // functionality.
  $details = array('module' => 'tripal_galaxy', // the name of the MODULE implementing the
                               // content type

  'unique_option' => '[tripal_galaxy.galaxy_id]');
  // This call adds the configuration form to your current form
  // This sub-form handles it's own validation & submit
  chado_add_admin_form_set_title($form, $form_state, $details);

  return system_settings_form($form);
}

/**
 * @ingroup tripal_galaxy
 */
function tripal_galaxy_admin_validate($form, &$form_state) {
}
