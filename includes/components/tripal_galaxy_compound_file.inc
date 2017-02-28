<?php
function _webform_defaults_compound_file() {
  return array(
    'name' => '',
    'form_key' => NULL,
    'pid' => 0,
    'weight' => 0,
    'value' => '',
    'required' => 0,
    'extra' => array(
      'width' => '',
      'maxlength' => '',
      'field_prefix' => '',
      'field_suffix' => '',
      'disabled' => 0,
      'unique' => 0,
      'title_display' => 0,
      'description' => '',
      'description_above' => FALSE,
      'placeholder' => '',
      'attributes' => array(),
      'private' => FALSE,
      'analysis' => FALSE,
    ),
  );
}

function _webform_theme_compound_file() {
  return array(
    'webform_display_compound_file' => array(
      'render element' => 'element',
    ),
  );
}
