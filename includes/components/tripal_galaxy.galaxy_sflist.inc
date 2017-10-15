<?php

/**
 *
 */
function _webform_defaults_galaxy_sflist() {
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
function _webform_theme_galaxy_sflist() {
  return array(
    'webform_display_galaxy_sflist' => array(
      'render element' => 'element',
      'file' => "tripal_galaxy.galaxy_sflist.inc",
      'path' => drupal_get_path('module', 'tripal_galaxy') . "/includes/components/",
    ),
  );
}

/**
 * Here you define your additional form elements using the form API to add to
 * the edit screen of the component.
 */
function _webform_edit_galaxy_sflist($component) {
 // TODO: revisit this function.
}

/**
 * Defines how the form element is sent to the webform itself.
 */
function _webform_render_galaxy_sflist($component, $value = NULL, $filter = TRUE) {
  $allowed_types = array();
  if (array_key_exists('filtering', $component['extra']) and
      array_key_exists('types', $component['extra']['filtering'])) {
    $allowed_types = array_values($component['extra']['filtering']['types']);
  }
  $element = array(
    '#type' => 'galaxy_sflist',
    '#title' => $component['name'],
    '#description' => array_key_exists('description', $component['extra']) ? $component['extra']['description'] : '',
    '#weight' => $component['weight'],
    '#theme_wrappers' => array('webform_element'),
    '#usage_type' => $component['form_key'],
    '#usage_id' => $component['nid'],
    '#usage_module' => 'tripal_galaxy',
    '#allowed_types' => $allowed_types,
    '#cardinality' => 0,
    '#required' => array_key_exists('required', $component) and  $component['required'] ? TRUE : FALSE,
  );
  return $element;
}
/**
 * Defines how the value is displayed from the component in the various
 * reporting from webform.
 */
function _webform_display_galaxy_sflist($component, $value, $format = 'html') {

  // Get any selected existing files.
  $eset = array();
  if  ($value['existing']) {
    $efids = $value['existing'];
    $eset = explode('|', $efids);
  }

  // Get any uploaded files.
  $name = '';
  foreach ($value as $key => $val) {
    if (preg_match('/^submitted_/', $key)) {
      $name = $key;
      break;
    }
  }
  $name = preg_replace('/[^\w]/', '_', $name);
  $ufids = $value[$name];
  $uset = explode('|', $ufids);

  // Merge the selected and uploaded files into a single value.
  $set = array();
  foreach ($eset as $val) {
    $set[] = $val;
  }
  foreach ($uset as $val) {
    $set[] = $val;
  }

  // Find the field ccontaining the fid.
  $fids = implode('|', $set);

  return array(
    '#title' => $component['name'],
    '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
    '#weight' => $component['weight'],
    '#theme' => 'webform_display_galaxy_sflist',
    '#theme_wrappers' => $format == 'html' ? array('webform_element') : array('webform_element_text'),
    '#format' => $format,
    '#value' => $fids,
    '#title' => !empty($component['name']) ? $component['name'] : 'Input Data List',
  );
}

/**
 *
 */
function _webform_csv_headers_galaxy_sflist($component, $export_options){

}

/**
 *
 */
function _webform_csv_data_galaxy_sflist($component, $export_options, $value){

}

/**
 * Theme hook for displaying the paired files.
 */
function theme_webform_display_galaxy_sflist($variables) {
  $element = $variables['element'];
  $prefix = $element['#format'] == 'html' ? '' : $element['#field_prefix'];
  $suffix = $element['#format'] == 'html' ? '' : $element['#field_suffix'];
  $value = $element['#format'] == 'html' ? check_plain($element['#value']) : $element['#value'];

  $set = explode('|', $value);
  $headers = array('', 'File Name');

  // Dirty fix to the extra row that's spawning
  $set = array_filter($set);

  $num_rows = count($set);
   for ($i = 0; $i < $num_rows; $i++) {
    $fid = $set[$i];
    $file = file_load($fid);
    $rows[] = array(
      ($i + 1),
      $file->filename,
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
