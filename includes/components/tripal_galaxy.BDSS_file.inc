<?php

/**
 *
 */
function _webform_defaults_BDSS_file() {
  return array(
    'name' => '',
    'form_key' => NULL,
    'pid' => 0,
    'weight' => 0,
    'value' => '',
    'required' => 0,
    'extra' => array(
      'file_URL' => '',
      'fid' => '',
    ),
  );
}

/**
 * Adds additional information in to hook_theme().
 */
function _webform_theme_BDSS_file() {
  return array(
    'webform_display_BDSS_file' => array(
      'render element' => 'element',
    ),
  );
}

/**
 * Here you define your additional form elements using the form API to add to
 * the edit screen of the component.
 */
function _webform_edit_BDSS_file($component) {
 // TODO: revisit this function.
}

/**
 * Defines how the form element is sent to the webform itself.
 */
function _webform_render_BDSS_file($component, $value = NULL, $filter = TRUE) {
  $file_URL = '';
  $fid = '';
  if (is_array($value)) {
    $file_URL = $filter ? webform_filter_xss($value['file_URL']) : $value['file_URL'];
    $fid = $filter ? webform_filter_xss($value['file_upload']) : $value['file_upload'];
  }

  $element = array();
  $element['#element_validate'] = array('_webform_edit_validate_BDSS_file');

  $element['instructions'] = array(
    '#type' => 'item',
    '#title' => $component['name'],
    '#markup' => array_key_exists('description', $component['extra']) ? $component['extra']['description'] : t('Please provide a URL for the file on a remote server or upload the file.'),
  );
  $element['file_URL'] = array(
    '#type' => 'textfield',
    '#title' => t('File URL'),
    '#default_value' => $file_URL,
    '#description' => 'File URL',
    '#title_display' => 'invisible',
  );
  $element['file_upload'] = array(
    '#type' => 'managed_file',
    '#title' => t('Upload a file'),
    '#default_value' => $fid,
    '#description' => 'Upload a file',
    '#title_display' => 'invisible',
  );
  return $element;
}

/**
 * Element validation callback. Ensure keys are not duplicated.
 */
function _webform_edit_validate_BDSS_file($element, &$form_state) {
  $is_required = $element['#webform_component']['required'];
  $file_URL = $element['file_URL']['#value'];
  $file_upload = $element['file_upload']['#value'];

  if (is_array($file_upload)) {
    $file_upload = $element['file_upload']['#value']['fid'];
  }
  if ($file_URL and $file_upload) {
    form_error($element, t('Please select a file URL or upload a file but not both.'));
  }

  if (!$file_upload and $file_URL and !valid_url($file_URL, TRUE)) {
    form_error($element['file_URL'], t('The URL is not valid. Please check for errors.'));
  }

  if ($is_required and !$file_URL and !$file_upload) {
    form_error($element, t('Please provide a file URL or upload a file.'));
  }
}
/**
 * Defines how the value is displayed from the component in the various
 * reporting from webform.
 */
function _webform_display_BDSS_file($component, $value, $format = 'html') {

}

/**
 *
 */
function _webform_csv_headers_BDSS_file($component, $export_options){

}

/**
 *
 */
function _webform_csv_data_BDSS_file($component, $export_options, $value){

}
