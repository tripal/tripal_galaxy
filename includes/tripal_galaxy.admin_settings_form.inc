<?php

/**
 * Implements Drupal's hook_form function, settings form for galaxy
 */
function tripal_galaxy_admin_settings_form($form, &$form_state) {
  $cron_default = variable_get('tripal_galaxy_history_age')/86400;
  if (!$cron_default) {
    $cron_default = 60;
  }

  $exp_default = variable_get('tripal_galaxy_cron_frequency') / 60;
  if (!$exp_default) {
    $exp_default = 60;
  }
  $form['#tree'] = true;

  $form['expiration'] = array(
    '#type' => 'textfield',
    '#title' => t('History Age Expiration (in days)'),
    '#description' => t('Delete histories on remote galaxy server after how many days?'),
    '#default_value' => $cron_default,
    '#size' => 60,
    '#maxlength' => 128,
    '#required' => true,
  );

  $form['cron'] = array(
    '#type' => 'textfield',
    '#title' => t('Galaxy cron run (in minutes)'),
    '#description' => t('How frequently the galaxy cron run should happen. We recoomend at least once per day as workflow expiration is tied into this.'),
    '#default_value' => $exp_default,
    '#size' => 60,
    '#maxlength' => 128,
    '#required' => true,
  );
  // Default submit
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Changes')
  );

  return $form;
}

/**
 * 
 *
 * @param $form
 * @param $form_state
 */
function tripal_galaxy_admin_settings_form_submit($form, &$form_state)
{
  if ($form_state['values']['expiration']) {
    variable_set('tripal_galaxy_history_age', round($form_state['values']['expiration'] * 60 * 60 * 24));
  }
  if ($form_state['values']['cron']) {
    variable_set('tripal_galaxy_cron_frequency', round($form_state['values']['cron'] * 60));
  }
  drupal_set_message(t('Your configuration has been saved.'));
}
