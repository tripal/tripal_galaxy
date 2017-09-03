<?php

/**
 *
 */
function _webform_defaults_galaxy_pflist() {
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
function _webform_theme_galaxy_pflist() {
  return array(
    'webform_display_galaxy_pflist' => array(
      'render element' => 'element',
    ),
  );
}

/**
 * Theme hook for displaying the paired files.
 */
function theme_webform_display_galaxy_pflist($variables) {
  $element = $variables['element'];
  $prefix = $element['#format'] == 'html' ? '' : $element['#field_prefix'];
  $suffix = $element['#format'] == 'html' ? '' : $element['#field_suffix'];
  $value = $element['#format'] == 'html' ? check_plain($element['#value']) : $element['#value'];

  list($set1, $set2) = explode(',', $value);
  $set1 = explode('|', $set1);
  $set2 = explode('|', $set2);

  $headers = array('Pair', 'File1', 'File2');
  $rows = array();
  $num_rows = count($set1);

  for ($i = 0; $i < $num_rows; $i++) {
    $fid1 = $set1[$i];
    $fid2 = $set2[$i];
    $file1 = file_load($fid1);
    $file2 = file_load($fid2);
    $rows[] = array(
      ($i + 1),
      $file1->filename,
      $file2->filename,
    );
  }
  $table = theme_table(array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(),
    'caption' => '',
    'colgroups' => array(),
    'sticky' => TRUE,
    'empty' => t('There are no files.')
  ));

  return $value !== '' ? ($prefix . $table . $suffix) : ' ';
}

/**
 * Here you define your additional form elements using the form API to add to
 * the edit screen of the component.
 */
function _webform_edit_galaxy_pflist($component) {
 // TODO: revisit this function.
}

/**
 * Defines how the form element is sent to the webform itself.
 */
function _webform_render_galaxy_pflist($component, $value = NULL, $filter = TRUE) {
  $allowed_types = array();
  if (array_key_exists('filtering', $component['extra']) and
      array_key_exists('types', $component['extra']['filtering'])) {
    $allowed_types = array_values($component['extra']['filtering']['types']);
  }
  $element = array(
    '#type' => 'galaxy_pflist',
    '#title' => $component['name'],
    '#description' => array_key_exists('description', $component['extra']) ? $component['extra']['description'] : '',
    '#weight' => $component['weight'],
    '#theme_wrappers' => array('webform_element'),
    '#usage_type' => $component['form_key'],
    '#usage_id' => $component['nid'],
    '#usage_module' => 'tripal_galaxy',
    '#allowed_types' => $allowed_types,
    '#cardinality' => 0,
    '#paired' => TRUE,
    '#required' => array_key_exists('required', $component) and  $component['required'] ? TRUE : FALSE,
  );
  return $element;
}
/**
 * Defines how the value is displayed from the component in the various
 * reporting from webform.
 */
function _webform_display_galaxy_pflist($component, $value, $format = 'html') {
  $filename = '';


  // Find the field ccontaining the fid.
  $fids = $value['existing'];

  return array(
    '#title' => $component['name'],
    '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
    '#weight' => $component['weight'],
    '#theme' => 'webform_display_galaxy_pflist',
    '#theme_wrappers' => $format == 'html' ? array('webform_element') : array('webform_element_text'),
    '#format' => $format,
    '#value' => $fids,
    '#title' => !empty($component['name']) ? $component['name'] : 'Input Paired Data',
  );
}

/**
 *
 */
function _webform_csv_headers_galaxy_pflist($component, $export_options){

}

/**
 *
 */
function _webform_csv_data_galaxy_pflist($component, $export_options, $value){

}
