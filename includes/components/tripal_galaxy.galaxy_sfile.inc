<?php

/**
 *
 */
function _webform_defaults_galaxy_sfile() {
  return array(
    'name' => '',
    'form_key' => NULL,
    'pid' => 0,
    'weight' => 0,
    'value' => '',
    'required' => 0,
    'extra' => array(),
  );
}

/**
 * Adds additional information in to hook_theme().
 */
function _webform_theme_galaxy_sfile() {
  return array(
    'webform_display_galaxy_sfile' => array(
      'render element' => 'element',
    ),
  );
}

/**
 * Here you define your additional form elements using the form API to add to
 * the edit screen of the component.
 */
function _webform_edit_galaxy_sfile($component) {
 // TODO: revisit this function.
}

/**
 * Defines how the form element is sent to the webform itself.
 */
function _webform_render_galaxy_sfile($component, $value = NULL, $filter = TRUE) {
  $allowed_types = array();
  if (array_key_exists('filtering', $component['extra']) and
      array_key_exists('types', $component['extra']['filtering'])) {
    $allowed_types = array_values($component['extra']['filtering']['types']);
  }
  $element = array(
    '#type' => 'galaxy_sfile',
    '#title' => $component['name'],
    '#description' => array_key_exists('description', $component['extra']) ? $component['extra']['description'] : '',
    '#weight' => $component['weight'],
    '#theme_wrappers' => array('webform_element'),
    '#usage_type' => $component['form_key'],
    '#usage_id' => $component['nid'],
    '#usage_module' => 'tripal_galaxy',
    '#allowed_types' => $allowed_types,
    '#cardinality' => 1,
    '#required' => array_key_exists('required', $component) and  $component['required'] ? TRUE : FALSE,
  );
  return $element;
}
/**
 * Defines how the value is displayed from the component in the various
 * reporting from webform.
 */
function _webform_display_galaxy_sfile($component, $value, $format = 'html') {
  $filename = '';

  // Find the field ccontaining the fid.
  $fid = NULL;
  $filename = 'No file selected';

  foreach ($value as $key => $val) {
    if (preg_match('/submitted.*' . $component['form_key'] . '_/', $key)) {
      $fid = $val;
    }
  }
  if (!$fid and array_key_exists('existing_fset', $value) ) {
    $fid = $value['existing_fset']['existing'];
  }
  if (!$fid and array_key_exists('site_wide_fset', $value) ) {
    $fid = $value['site_wide_fset']['site_wide'];
  }
  if (!$fid and array_key_exists('data_collection_fset', $value) ) {
    $fid = $value['data_collection_fset']['data_collection'];
  }
  if ($fid) {
    $file = file_load($fid);
    $filename = $file->filename;
  }

  return array(
    '#title' => $component['name'],
    '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
    '#weight' => $component['weight'],
    '#theme' => 'webform_display_textfield',
    '#theme_wrappers' => $format == 'html' ? array('webform_element') : array('webform_element_text'),
    '#format' => $format,
    '#value' => $filename,
    '#title' => !empty($component['name']) ? $component['name'] : 'Input Data File',
  );
}

/**
 *
 */
function _webform_csv_headers_galaxy_sfile($component, $export_options){

}

/**
 *
 */
function _webform_csv_data_galaxy_sfile($component, $export_options, $value){

}
