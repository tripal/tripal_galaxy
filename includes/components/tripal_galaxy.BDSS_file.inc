<?php

/**
 * @file
 * Webform component for a BDSS file.
 */

// phpcs:disable Drupal.NamingConventions.ValidFunctionName.InvalidName

/**
 * The defaults function for a webform component.
 */
function _webform_defaults_BDSS_file() {
  return array(
    'name' => '',
    'form_key' => NULL,
    'pid' => 0,
    'weight' => 0,
    'value' => '',
    'required' => 0,
    'extra' => [],
  );
}

/**
 * The theme function for a webform component.
 *
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
 * The edit function for a webform component.
 *
 * Here you define your additional form elements using the form API to add to
 * the edit screen of the component.
 */
function _webform_edit_BDSS_file($component) {
  // TODO: revisit this function.
}

/**
 * The render function for a webform component.
 *
 * Defines how the form element is sent to the webform itself.
 */
function _webform_render_BDSS_file($component, $value = NULL, $filter = TRUE) {
  $element = array(
    '#type' => 'BDSS_file',
    '#title' => $component['name'],
    '#description' => array_key_exists('description', $component['extra']) ? $component['extra']['description'] : '',
    '#weight' => $component['weight'],
    '#theme_wrappers' => array(
      'webform_element',
    ),
  );
  return $element;
}

/**
 * The display function for a webform component.
 *
 * Defines how the value is displayed from the component in the various
 * reporting from webform.
 */
function _webform_display_BDSS_file($component, $value, $format = 'html') {
  $url = '';

  $file_URL = $value['file_URL'];
  $fid = $value['file_upload'];

  if ($file_URL) {
    $url = $file_URL;
  }
  if ($fid) {
    $file = file_load($fid);
    $url = file_create_url($file->uri);
  }

  return array(
    '#title' => $component['name'],
    '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
    '#weight' => $component['weight'],
    '#theme' => 'webform_display_textfield',
    '#theme_wrappers' => $format == 'html' ? array(
      'webform_element',
    ) : array(
      'webform_element_text',
    ),
    '#format' => $format,
    '#value' => $url,
  );
}

/**
 * The CSV headers function for a webform component.
 */
function _webform_csv_headers_BDSS_file($component, $export_options) {
}

/**
 * The CSV headers function for a webform component.
 */
function _webform_csv_data_BDSS_file($component, $export_options, $value) {
}

// phpcs:enable Drupal.NamingConventions.ValidFunctionName.InvalidName
