<?php

/**
 * @file
 * Settings form for the site admin.
 */

/**
 * The settings form.
 */
function tripal_galaxy_admin_settings_form($form, &$form_state) {
  $cron_default = variable_get('tripal_galaxy_history_days_limit', 60);
  $exp_default = variable_get('tripal_galaxy_cron_hours', 24);

  $form['#tree'] = TRUE;

  $form['expiration'] = array(
    '#type' => 'textfield',
    '#title' => t('History Age Expiration (in days)'),
    '#description' => t('Delete histories on remote galaxy server after how many days?'),
    '#default_value' => $cron_default,
    '#size' => 60,
    '#maxlength' => 128,
    '#required' => TRUE,
  );

  $form['cron'] = array(
    '#type' => 'textfield',
    '#title' => t('Galaxy cron run (in hours)'),
    '#description' => t('Specify how frequently the galaxy cron run should happen. We recommend at least once per day (e.g. 24).'),
    '#default_value' => $exp_default,
    '#size' => 60,
    '#maxlength' => 128,
    '#required' => TRUE,
  );
  // Default submit.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Changes'),
  );

  return $form;
}

/**
 * Form submit.
 */
function tripal_galaxy_admin_settings_form_submit($form, &$form_state) {
  if ($form_state['values']['expiration']) {
    variable_set('tripal_galaxy_history_days_limit', round($form_state['values']['expiration']));
  }
  if ($form_state['values']['cron']) {
    variable_set('tripal_galaxy_cron_hours', round($form_state['values']['cron']));
  }
  drupal_set_message(t('Your configuration has been saved.'));
}
