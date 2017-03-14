<?php

/**
 *
 */
function _webform_defaults_fixed_value() {
  return array(
    'name' => '',
    'form_key' => NULL,
    'pid' => 0,
    'weight' => 0,
    'value' => '',
    'required' => 0,
    'extra' => array(
    ),
  );
}

/**
 * Adds additional information in to hook_theme().
 */
function _webform_theme_fixed_value() {
  return array(
    'webform_display_fixed_value' => array(
      'render element' => 'element',
    ),
  );
}

/**
 * Here you define your additional form elements using the form API to add to
 * the edit screen of the component.
 */
function _webform_edit_fixed_value($component) {
  $form = array();

  $select_options = array();
  if(!empty($component['extra']['items'])){
    $items = $component['extra']['items'];

    $options = preg_split('/\n/', $items);
    foreach ($options as $option) {
      list($key, $val) = explode('|', $option);
      $select_options[$key] = $val;
    }
  }

  $form['value'] = array(
    '#type' => 'select',
    '#title' => t('Default value'),
    '#default_value' => $component['value'],
    '#options' => $select_options,
  );

  return $form;
}

/**
 * Defines how the form element is sent to the webform itself.
 */
function _webform_render_fixed_value($component, $value, $filter = TRUE) {

  // Get the human readable value
  $human_readable = $component['value'];

  if(!empty($component['extra']['items'])){
    $items = $component['extra']['items'];

    $options = preg_split('/\n/', $items);
    foreach ($options as $option) {
      if (preg_match('/\|/', $option)) {
        list($key, $val) = explode('|', $option);
        if ($key == $component['value']) {
          $human_readable = $val;
        }
      }
    }
  }

  $element = array(
    '#type' => 'fixed_value',
    '#value' => $component['value'],
    '#weight' => $component['weight'],
    '#title' => $component['name'],
    '#markup' => $human_readable,
    '#weight' => $component['weight'],
    '#theme_wrappers' => array('webform_element'),
  );
  return $element;

}
/**
 * Defines how the value is displayed from the component in the various
 * reporting from webform.
 */
function _webform_display_fixed_value($component, $value, $format = 'html') {
  // Get the human readable value
  $human_readable = $component['value'];

  if(!empty($component['extra']['items'])){
    $items = $component['extra']['items'];

    $options = preg_split('/\n/', $items);
    foreach ($options as $option) {
      if (preg_match('/\|/', $option)) {
        list($key, $val) = explode('|', $option);
        if ($key == $component['value']) {
          $human_readable = $val;
        }
      }
    }
  }

  return array(
    '#title' => $component['name'],
    '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
    '#weight' => $component['weight'],
    '#theme' => 'webform_display_textfield',
    '#theme_wrappers' => $format == 'html' ? array('webform_element') : array('webform_element_text'),
    '#format' => $format,
    '#value' => $human_readable,
  );
}



/**
 *
 */
function _webform_csv_headers_fixed_value($component, $export_options){
  $header = array();
  $header[0] = '';
  $header[1] = '';
  $header[2] = $export_options['header_keys'] ? $component['form_key'] : $component['name'];
  return $header;

}

/**
 *
 */
function _webform_csv_data_fixed_value($component, $export_options, $value){
  return isset($value[0]) ? $value[0] : '';
}

/**
 * Implements _webform_table_component().
 */
function _webform_table_fixed_value($component, $value) {
  return empty($value[0]) ? '' : check_plain($value[0]);
}

/**
 * Implements _webform_action_set_component().
 */
function _webform_action_set_fixed_value($component, &$element, &$form_state, $value) {
  $element['#value'] = $value;
  form_set_value($element, $value, $form_state);
}
