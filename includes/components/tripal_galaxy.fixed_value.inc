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
  // TODO: revisit this function.
}

/**
 * Defines how the form element is sent to the webform itself.
 */
function _webform_render_fixed_value($component, $value, $filter = TRUE) {

  // Get the human readable value
  $human_readable = $component['value'];

  if(!empty($component['extra']['items'])){
    $items = $component['extra']['items'];
    $pattern = $component['value'];

    $value_key = array();
    $newline = array();

    preg_match('/('. $pattern .')/', $items, $value_key, PREG_OFFSET_CAPTURE);
    preg_match('/(\n)/', $items, $newline, PREG_OFFSET_CAPTURE, $value_key[0][1]);

    $length = $value_key[0][1] + strlen($pattern) + 1;
    $human_readable = substr($items, $length, ($newline[0][1] - $length));
  }

  $element = array();

  $element['fixed_value'] = array(
    '#type' => 'value',
    '#value' => $component['value'],
  );
  $element['display'] = array(
    '#type' => 'item',
    '#title' => $component['name'],
    '#markup' => $human_readable,
  );
  return $element;
}
/**
 * Defines how the value is displayed from the component in the various
 * reporting from webform.
 */
function _webform_display_fixed_value($component, $value, $format = 'html') {

}

/**
 *
 */
function _webform_csv_headers_fixed_value($component, $export_options){

}

/**
 *
 */
function _webform_csv_data_fixed_value($component, $export_options, $value){

}
