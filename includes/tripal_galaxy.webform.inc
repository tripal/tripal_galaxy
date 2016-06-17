<?php

/**
 * The different tool input parameters:
 * https://github.com/galaxyproject/galaxy/blob/dev/client/galaxy/scripts/mvc/form/form-parameters.js#L96
 */


/**
 * The base function to submitted_list()
 */
function tripal_galaxy_build_webform($galaxy_id, $workflow_id) {

  // The array of components for the webform.
  $components = array();

  // Obtain the galaxy server details using the galaxy id.
  $sql = "SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id";
  $galaxy_server = db_query($sql, array(':galaxy_id' => $galaxy_id));
  $galaxy_server = $galaxy_server->fetchObject ();

  // Obtain the hostname and port from the url.
  $matches = array ();
  $regex = "((https|http):\/\/)?"; // SCHEME
  $regex .= "([a-z0-9-.]*)"; // Host or IP
  $regex .= "(:)?([0-9]*)?"; // Port
  preg_match ( "/^$regex$/", $galaxy_server->url, $matches );
  $hostname = $matches[3];
  $port = $matches[5];

  // Load the GalaxyLib-PHP library into scope and create the Galaxy instance.
  $library = libraries_load('GalaxyLib-PHP');
  $galaxy_instance = new GalaxyInstance($hostname, $port);

  // Check to make sure we have a valid connection.
  if($galaxy_instance->getVersion() == NULL){
    drupal_set_message(t('Cannot connect to galaxy instance'), 'error');
    return;
  }

  // Set the API for this galaxy instance.
  $galaxy_instance->setAPIKey($galaxy_server->api_key);

  // Create the workflow instance.
  $workflow_instance = new GalaxyWorkflows($galaxy_instance);
  $workflow = $workflow_instance->show($workflow_id);

  // Select a target history by using the first history owned by the user.
  // The history_id is required for retrieving tool information for a workflow.
  // Also, the workflow must be owned by the user.
  $history_instance = new GalaxyHistories($galaxy_instance);
  $histories = $history_instance->index();
  $target_history = $histories[0];

  // Make sure we have a valid workflow.
  if ($workflow == NULL || !is_array($workflow) || !array_key_exists('steps', $workflow)) {
    drupal_set_message(t('Unrecognized response from Galaxy server'), 'error');
    return;
  }

  dpm($workflow);
  // Construct web form components based on workflow steps.
  $i = 1;
  foreach ($workflow['steps'] as $step_index => $step) {

    // Get the tool for this step.  Some steps don't have tools so the tool_id
    // stays as null if there isn't one.
    $tool = NULL;
    $tool_id = $step['tool_id'];
    if ($tool_id) {
      $tool_instance = new GalaxyTools($galaxy_instance);
      $tool = $tool_instance->build(array(
        'tool_id' => urlencode ($tool_id),
        'history_id' => $target_history['id']
      ));

      // Make sure tool informaiton is correct.
      if (!$tool) {
        drupal_set_message('Cannot retrieve tool information from Galaxy: ' . $tool_id, 'Error' );
        return;
      }
      dpm($tool);
    }

    // Each step is contained in a fieldset. We'll name the field set after
    // it's step and if a tool is present we'll include the tool info.
    $cid = count($components) + 1;
    $fieldset_key = "Step_" . $i . "_fieldset";
    $fieldset_name = 'Step ' . $i;
    $fieldset_desc = '';
    if ($tool) {
      $fieldset_name .=  ': ' . $tool['name'] . ' v' .  $tool['version'];
      $fieldset_desc = $tool['description'];
    }

    // Add the fieldset to the components list.
    $components[] = array(
      'cid' => $cid,
      'pid' => '0',
      'form_key' => $fieldset_key,
      'name' => $fieldset_name,
      'type' => 'fieldset',
      'value' => '',
      'extra' => array(
        'description_above' => 1,
        'private' => 0,
        'css_classes' => '',
        'title_display' => 0,
        'collapsible' => 0,
        'collapsed' => 0,
        'description' => $fieldset_desc,
      ),
      'required' => '0',
      'weight' => $cid,
    );

    // Add the tool help as a collapsed fieldset
    if ($tool) {
      $icid = count($components) + 1;
      $components[] = array(
        'cid' => $icid,
        'pid' => $cid,
        'form_key' => $fieldset_key . '_help',
        'name' => 'About ' . $tool['name'],
        'type' => 'fieldset',
        'value' => '',
        'extra' => array(
          'description_above' => 1,
          'private' => 0,
          'css_classes' => '',
          'title_display' => 1,
          'collapsible' => 1,
          'collapsed' => 1,
        ),
        'required' => '0',
        'weight' => $icid,
      );
      $hcid = count($components) + 1;
      $components[] = array(
        'cid' => $hcid,
        'pid' => $icid,
        'form_key' => $fieldset_key . '_help_details',
        'name' => $tool['name'] . ' Help',
        'type' => 'markup',
        'value' => $tool['help'],
        'extra' => array(
          'format' => 'full_html',
          'private' => 0,
          'css_classes' => '',
          'title_display' => 0,
        ),
        'required' => '0',
        'weight' => $icid,
      );
    }

    // Add in the tool and it's inputs to the webform.
    if (!$tool) {
      tripal_galaxy_add_webform_non_tool_input($components, $step, $i, $cid);
    }
    else {
      // Iterate over tool inputs.
      foreach ($tool["inputs"] as $input) {
        // If there is no type skip this input
        if (!array_key_exists('type', $input)) {
          dpm("@@@ @@@ @@@ Tool input contains no type! Tool information: @@@ @@@ @@@");
          dpm($tool);
          continue;
        }

        // Add appropriate webform components to match the inputs.
        tripal_galaxy_add_webform_tool_input($components, $input, $step, $i, $cid);
      }
    }
    // Increment the step counter;
    $i++;
  }
  dpm($components);

  // Default values for the node
  $node = new stdClass ();
  $node->type = 'webform';
  node_object_prepare($node);
  $node->title = $workflow['name'];
  $node->language = 'en';
  $node->body[LANGUAGE_NONE][0]['value'] = '';
  $node->body[LANGUAGE_NONE][0]['format'] = 'full_html';
  $node->uid = 1;
  $node->promote = 0;
  $node->comment = 0;

    // Attach the webform to the node.
  $node->webform = array (
    'confirmation' => '',
    'confirmation_format' => NULL,
    'redirect_url' => '',
    'status' => '1',
    'block' => '0',
    'teaser' => '0',
    'allow_draft' => '0',
    'auto_save' => '0',
    'submit_notice' => '1',
    'confidential' => '0',
    'submit_text' => '',
    'submit_limit' => '-1', // User can submit more than once.
    'submit_interval' => '-1',
    'total_submit_limit' => '-1',
    'total_submit_interval' => '-1',
    'progressbar_bar' => '1',
    'progressbar_page_number' => '0',
    'progressbar_percent' => '0',
    'progressbar_pagebreak_labels' => '1',
    'progressbar_include_confirmation' => '1',
    'progressbar_label_first' => 'Start',
    'progressbar_label_confirmation' => 'Complete',
    'preview' => '0',
    'preview_next_button_label' => '',
    'preview_prev_button_label' => '',
    'preview_title' => '',
    'preview_message' => '',
    'preview_message_format' => NULL,
    'record_exists' => TRUE,
    'roles' => array (
     '1', '2',
    ),
    'emails' => array(),
    'components' => $components
  );

  // Save the node.
  node_save($node);
}

/**
 *
 * @param $components
 * @param $step
 * @param $step_index
 * @param $pid
 */
function tripal_galaxy_add_webform_non_tool_input(&$components, $step, $step_index, $pid = 0) {

  $cid = count($components) + 1;
  switch ($step['type']) {
    case 'data_input':
      $components[] = array(
        'cid' => $cid,
        'pid' => $pid,
        'name' => $step['tool_inputs']['name'],
        'form_key' => 'dataset_' . $step_index,
        'type' => 'file',
        'mandatory' => 1,
        'filtering' => array(
          'types' => array(
            'bed',
            'bam',
            'blast'
          ),
          'addextensions' => '',
          'size' => file_upload_max_size()
        ),
        'extra' => array(
          'description' => $step['annotation'],
        ),
        'weight' => $cid
      );
      break;
    default:
      break;
  }
}


/**
 * A function to construct a web form component based on
 *   A given tool input.
 *
 * @param $tool_input
 *   The tool input to build the component for. Must have Key 'type'.
 * @param $index
 *   The index to help set the tool name to.
 *
 * @return
 *   An array suitable for the 'components' webform array
 *   False otherwise.
 */
function tripal_galaxy_add_webform_tool_input(&$components, $tool_input, $step, $step_index, $pid = 0) {

  // Initiate variables. By default, these variables will create
  // A drop-down list webform component.
  $options = array();
  $component = array ();
  $is_list = 1;
  $webform_items = "0|No Options";
  $webform_value = "";
  $webform_type = 'select';
  $input_name = '';
  $is_required = 0;
  $extra = array(
    'title_display' => 'before',
    'description_above' => 1,
    'private' => 0,
    'items' => $webform_items,
    'aslist' => $is_list,
    'multiple' => 0,
  );

  // If the tool_input contains a value of test_param then set the tool input
  // equal to that.
  if(array_key_exists("test_param", $tool_input)){
    $tool_input = $tool_input['test_param'];
  }

  // Set the default value accordingly.
  if(array_key_exists('value', $tool_input)){
    $webform_value = $tool_input['value'];
  }

  // Find the options.
  if (array_key_exists('options', $tool_input)) {
    $options = $tool_input['options'];
  }

  // Is this field required?
  if (array_key_exists('optional', $tool_input) and $tool_input['optional']) {
    $is_required = 1;
  }

  // Is this a private field?
  if (array_key_exists('hidden', $tool_input) and $tool_input['hidden']) {
    $extra['private'] = 1;
  }

  if (array_key_exists('multiple', $tool_input) and $tool_input['multiple']) {
    $extra['multiple'] = 1;
  }

  if (array_key_exists('help', $tool_input) and $tool_input['help']) {
    $extra['description'] = $tool_input['help'];
  }

  // Add options to the webform items list. This loop is tricky,
  // but its essentially grabbing the first non-array item in each option.
  $x = 0;
  foreach ($options as $option) {
    if (count($option) == 0) {
      continue;
    }

      // Iterate through the subArrays until we find our first string value.
    while (is_array($option)) {

      // if the 'name' element exists, use this as our string value
      if (array_key_exists('name', $option)) {
        $option = $option['name'];
        continue;
      }

      // Let the option equal the first element in the option array
      foreach ($option as &$inner_option) {
        $option = $inner_option;
        break;
      }
    }

    // Add the option to the webform_items in the form:
    // 'key1|value1 \n key2|value2'.
    $webform_items .= $option ."_" . $x . "|" . $option ."\n";
    $x++;
  }


  // Extract tool input form information to a drupal webform component.
  switch ($tool_input['type']) {
    case 'text':
      // Text should be the webform equivalent of textfield.
      $webform_type = 'textfield';
      if(array_key_exists('value', $tool_input)){
        $webform_value = $tool_input['value'];
      }
      break;

    case 'select':
      // Select should be the default list drop-down. Unless it has a display
      // of Checkbox
      if ($tool_input['display'] == "checkboxes") {
        $extra['aslist'] = 0;
        $extra['multiple'] = 1;
      }
      break;

    case 'data_column':
      // Data_column should be the default list drop-down.
      $extra['multiple'] = 0;
      break;

    case 'genomebuild':
      break;

    case 'data':
      // For the most part, this should be the default drop down list webform
      break;

    case 'data_collection':
      break;

    case 'integer':
      break;

    case 'float':
      break;

    case 'boolean':
      // Boolean should have a single true/false value. It is also likely to
      // not have 'options' So we must invent our own
      $extra['items'] = "t|True\nf|False";
      $extra['aslist'] = 0;
      break;

    case 'drill_down':
      break;

    case 'color':
      break;

    case 'hidden':
      break;

    case 'hidden_data':
      break;

    case 'baseurl':
      break;

    case 'library_data':
      break;

    case 'ftpfile':
      break;

    case 'repeat':
      break;

    default :
      drupal_set_message('Unhandled tool input type: ' . $tool_input['type'], 'error');
      break;
  }

  // Get the input name.
  $input_name = NULL;
  if (array_key_exists('label', $tool_input)) {
    $input_name = $tool_input['label'];
  }
  else if(array_key_exists('title', $tool_input)) {
    $input_name = $tool_input['title'];
  }

  // Default component. Defaults to a drop-down list component.
  $cid = count($components) + 1;
  $components[] = array (
    'cid' => $cid,
    'pid' => $pid,
    'name' => $input_name,
    'form_key' => "Step_" . $step_index ."_" . $cid,
    'type' => $webform_type,
    'value' => $webform_value,
    'extra' => $extra,
    'weight' => $cid,
    'required' => $is_required,
  );
}
