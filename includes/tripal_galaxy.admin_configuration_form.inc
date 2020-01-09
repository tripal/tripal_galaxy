<?php

/**
 * @file
 * The configuration form for adding/deleting a Galaxy server.
 */

/**
 * The form for adding a new Galaxy server.
 */
function tripal_galaxy_admin_configuration_form($form, $form_state, $galaxy_id = NULL) {
  // Set the breadcrumb.
  $breadcrumb = array();
  $breadcrumb[] = l(t('Home'), '<front>');
  $breadcrumb[] = l(t('Administration'), 'admin');
  $breadcrumb[] = l(t('Tripal'), 'admin/tripal');
  $breadcrumb[] = l(t('Extensions'), 'admin/tripal/extension');
  $breadcrumb[] = l(t('Galaxy'), 'admin/tripal/extension/galaxy');
  drupal_set_breadcrumb($breadcrumb);

  $default_server_name = "";
  $default_user_name = "";
  $default_description = "";
  $default_api_key = "";
  $default_url = "";

  // If a galaxy id is set, feel free to declare all the default values with the
  // appropriate
  // Galaxy instance.
  if ($galaxy_id != NULL) {
    $sql = "SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id";
    $galaxy = db_query($sql, array(
      ':galaxy_id' => $galaxy_id,
    ));
    $galaxy = $galaxy->fetchObject();
    if ($galaxy != NULL) {
      $default_api_key = $galaxy->api_key;
      $default_description = $galaxy->description;
      $default_server_name = $galaxy->servername;
      $default_user_name = $galaxy->username;
      $default_url = $galaxy->url;
    }
  }

  // The galaxy id.
  $form['galaxy_id'] = array(
    '#type' => 'value',
    '#value' => $galaxy_id,
  );

  // Server Name.
  $form['servername'] = array(
    '#title' => t('Galaxy Server Name'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value' => $default_server_name,
    '#description' => 'Please provide the name of the remote Galaxy Server',
    '#required' => TRUE,
  );

  // Description.
  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#default_value' => $default_description,
    '#description' => 'Provide any additional details about this server.',
    '#required' => FALSE,
  );

  // URL.
  $form['url'] = array(
    '#title' => t('URL'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value' => $default_url,
    '#description' => 'The URL for the remote Galaxy server.',
    '#required' => TRUE,
  );

  // User Name.
  $form['username'] = array(
    '#title' => t('User Name'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value' => $default_user_name,
    '#description' => 'The user name for the Galaxy server.
        This username is used to launch all jobs by default.
        If this field is left blank then it is expected that the user
        has an account on the Galaxy server and will provide their username
        when executing workflows.',
    '#required' => FALSE,
  );

  // API Key.
  $form['api_key'] = array(
    '#title' => t('API Key'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value' => $default_api_key,
    '#description' => "The API key for the user name specified above.
        If this field is left blank then it is expected that the user
        will provide their own API key when submitting a job.",
    '#required' => FALSE,
  );

  // Default submit.
  $form['submit'] = array(
    '#type' => 'submit',
    '#name' => 'submit',
    '#value' => t('Submit'),
  );

  // Test Connection.
  $form['test_connect'] = array(
    '#type' => 'submit',
    '#name' => 'test_connect',
    '#value' => t('Test Connection'),
  );

  // Delete.
  if ($galaxy_id) {
    $form['delete'] = array(
      '#type' => 'markup',
      '#markup' => l(t('Delete'), 'admin/tripal/extension/galaxy/delete/' . $galaxy_id),
    );
  }

  return $form;
}

/**
 * The validate hook.
 */
function tripal_galaxy_admin_configuration_form_validate($form, &$form_state) {

  // Check to see if we are deleting an instance.
  if ($form_state['clicked_button']['#name'] == 'delete') {
    $server = db_query('tripal_galaxy', 'tg')->fields('tg')
      ->condition('servername', $form_state['values']['servername'])
      ->execute()
      ->fetchObject();

    if ($server) {
      // If there is no entry with this servername return False.
      form_set_error('servername', t('A galaxy instance with that servername already exists. Please choose another name.'));
      return FALSE;
    }
  }
}

/**
 * The submit hook.
 */
function tripal_galaxy_admin_configuration_form_submit($form, &$form_state) {
  global $user;

  // Initialize the data to be sent to the tripal_galaxy table.
  $data = array(
    'servername' => trim($form_state['values']['servername']),
    'description' => trim($form_state['values']['description']),
    'url' => trim($form_state['values']['url']),
    'username' => trim($form_state['values']['username']),
    'api_key' => trim($form_state['values']['api_key']),
    'uid' => trim($user->uid),
  );

  if ($form_state['clicked_button']['#name'] == 'submit') {
    // If there is a galaxy ID this will be an edit.
    if ($form_state['values']['galaxy_id']) {
      $data['galaxy_id'] = $form_state['values']['galaxy_id'];
      $galaxy = drupal_write_record('tripal_galaxy', $data, 'galaxy_id');
    }
    // Otherwise it's an insert.
    else {
      $galaxy = tripal_galaxy_add_galaxy($data);
    }

    // Insert the new Galaxy Instance into Drupal, return where appropriate.
    if (!$galaxy) {
      drupal_set_message(t('Problem inserting galaxy instance'), 'error');
      return;
    }

    // Default message.
    drupal_set_message(t('Galaxy instance has been added'));
    drupal_goto("/admin/tripal/extension/galaxy");
  }

  // Else if we are testing the connection.
  else {
    $form_state['rebuild'] = TRUE;

    $connect = tripal_galaxy_split_url($data['url']);
    tripal_galaxy_test_connection($connect, TRUE);
  }
}

/**
 * Form to deletes a galaxy instance.
 */
function tripal_galaxy_admin_delete_galaxy_instance_form($form, &$form_state, $galaxy_id) {
  $form = array();

  $galaxy = db_select('tripal_galaxy', 'tg')->fields('tg')
    ->condition('galaxy_id', array(
      $galaxy_id,
    ))
    ->execute()
    ->fetchObject();

  $form['galaxy_id'] = array(
    '#type' => 'value',
    '#value' => $galaxy_id,
  );

  return confirm_form($form, t('Confirm removal of the galaxy instance: !servername?',
    ['!servername' => $galaxy->servername]),
    'admin/tripal/extension/galaxy', t('WARNING: removal of a Galaxy instance will delete all workflows that have been executed by users of this site. Are you sure you want to do this?'));
}

/**
 * Delete form submit.
 */
function tripal_galaxy_admin_delete_galaxy_instance_form_submit($form, &$form_state) {
  $galaxy_id = $form_state['values']['galaxy_id'];

  try {
    // Remove any submissions.
    $sql = "
      DELETE FROM {tripal_galaxy_workflow_submission}
      WHERE galaxy_workflow_id IN
        (SELECT galaxy_workflow_id
         FROM {tripal_galaxy_workflow}
         WHERE galaxy_id = :galaxy_id)";
    $args = array(
      ':galaxy_id' => $galaxy_id,
    );
    $galaxy = db_query($sql, $args);

    // Remove the galaxy workflows.
    $sql = "DELETE FROM {tripal_galaxy_workflow} WHERE galaxy_id = :galaxy_id";
    $args = array(
      ':galaxy_id' => $galaxy_id,
    );
    $galaxy = db_query($sql, $args);

    // Remove the Galaxy instance itself.
    $sql = "DELETE FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id";
    $args = array(
      ':galaxy_id' => $galaxy_id,
    );
    $galaxy = db_query($sql, $args);

    drupal_set_message(t('Galaxy instance, workflows and submissions have all been removed.'));
  }
  catch (ErrorException $e) {
    drupal_set_message(t('Error deleting record'), 'error');
  }
  drupal_goto("/admin/tripal/extension/galaxy");
}
