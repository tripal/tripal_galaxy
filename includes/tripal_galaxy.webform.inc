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

  // The conditionals array for the webform
  $conditionals = array();

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

  // Load the blend4php library into scope and create the Galaxy instance.
  $library = libraries_load('blend4php');
  $galaxy_instance = new GalaxyInstance($hostname, $port);

  // Check to make sure we have a valid connection.
  if($galaxy_instance->getVersion() == NULL){
    drupal_set_message(t('Cannot connect to galaxy instance'), 'error');
    return FALSE;
  }

  // Set the API for this galaxy instance.
  $galaxy_instance->setAPIKey($galaxy_server->api_key);

  // Create the workflow instance.
  $workflow_instance = new GalaxyWorkflows($galaxy_instance);
  $workflow = $workflow_instance->show(array('workflow_id' => $workflow_id));
  if (!$workflow) {
    drupal_set_message('Cannot find workflow. ' . $galaxy_instance->getErrorMessage(), 'error');
    return FALSE;
  }
  dpm($workflow);

  // Select a target history by using the first history owned by the user.
  // The history_id is required for retrieving tool information for a workflow.
  // Also, the workflow must be owned by the user.
  $history_instance = new GalaxyHistories($galaxy_instance);
  $histories = $history_instance->index();
  $target_history = $histories[0];

  // Construct web form components based on workflow steps.
  $i = 1;
  $steps = $workflow['steps'];
  foreach ($steps as $step_index => $step) {

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
        return FALSE;
      }
      if ($tool['name'] == 'TopHat') {
        dpm($tool);
      }
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
        'collapsible' => 1,
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

    // If this step isn't a tool then handle it appropriately.
    if (!$tool) {
      tripal_galaxy_add_webform_non_tool_input($components, $step, $i, $cid);
    }
    // Otherwise, iterate over tool inputs and add webform components.
    else {
      foreach ($tool["inputs"] as $tool_input) {
        tripal_galaxy_add_webform_tool_input($components, $conditionals, $tool_input, $steps, $i, $cid);
      }
    }
    // Increment the step counter;
    $i++;
  }
  dpm($components);
  dpm($conditionals);

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
    'components' => $components,
    'conditionals' => $conditionals
  );

  // Save the node.
  node_save($node);
  return TRUE;
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
function tripal_galaxy_add_webform_tool_input(&$components, &$conditionals, $tool_input, $steps, $step_index, $pid = 0) {

  // Initiate variables. By default, these variables will create
  // A drop-down list webform component.
  $options = array();
  $component = array ();
  $is_list = 1;
  $webform_value = "";
  $webform_type = 'select';
  $input_name = '';
  $is_required = 0;
  $extra = array(
    'title_display' => 'before',
    'description_above' => 1,
    'private' => 0,
    'items' => "0|-Select-",
    'aslist' => $is_list,
    'multiple' => 0,
  );

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
      // Data appears to be a file selection of files that already
      // exists in the user's history.
      $webform_type = 'select';
      foreach ($tool_input['options'] as $key => $vals) {
        foreach ($vals as $details) {
          $extra['items'] .= $details['id'] . '|' . $details['name'] . "\n";
        }
      }
      $webform_value = '';
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

    case 'conditional':
      // A conditional case contains nested components but those components
      // should only be shown when specific selections are made in other
      // components. We will recursively add all the components and then
      // create the conditional rules that webforms will use to show the
      // conditional fields.

      // First add the test param as this is used for the conditional
      tripal_galaxy_add_webform_tool_input($components, $conditionals, $tool_input['test_param'], $steps, $step_index, $pid);
      $num_components = count($components) - 1;
      $source_id = $components[$num_components]['cid'];

      // Add the case paramters.
      foreach ($tool_input['cases'] as $case) {
        foreach ($case['inputs'] as $cindex => $input) {

          tripal_galaxy_add_webform_tool_input($components, $conditionals, $input, $steps, $step_index, $pid);
          $num_components = count($components) - 1;
          $target_id = $components[$num_components]['cid'];

          $conditionals[] = array(
            'andor' => NULL,
            'weight' => $cindex,
            'rules' => array(
              array(
                'source_type' => 'component',
                'source' => $source_id,
                // element will be listening
                'operator' => 'equal',
                // This is the option for which this conditional applies.
                'value' => $case['value'],
              ),
            ),
            // What will happen when the conditional is met
            'actions' => array(
              array(
                'target_type' => 'component',
                'target' => $target_id,
                'action' => 'show',
                'argument' => '',
              ),
            ),
          );
        }
      }

      // We won't add a 'conditional' as a component so we can short circuit
      // this function and return.
      return;
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


  // If this field recieves it's value from a previous step then we want
  // to set the type as markup and print that information
  $tool_name = $tool_input['name'];
  if (array_key_exists($tool_name, $steps[$step_index-1]['input_steps'])){
    $source_step = $steps[$step_index-1]['input_steps'][$tool_name]['source_step'];
    $step_output = $steps[$step_index-1]['input_steps'][$tool_name]['step_output'];
    $webform_type = 'markup';
    $extra['format'] = 'full_html';
    $webform_value =  "<label>" . $input_name . "</label>" .
      "Output from step " . ($source_step + 1);
  }


  // Default component. Defaults to a drop-down list component.
  $cid = count($components) + 1;
  $components[] = array(
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
