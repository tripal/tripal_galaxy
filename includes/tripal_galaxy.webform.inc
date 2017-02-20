<?php

/**
 * The different tool input parameters:
 * https://github.com/galaxyproject/galaxy/blob/dev/client/galaxy/scripts/mvc/form/form-parameters.js#L96
 */


/**
 * The base function to submitted_list()
 */
function tripal_galaxy_build_webform($galaxy_id, $workflow_id)
{
  // Obtain the galaxy server details using the galaxy id.
  $sql = "SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id";
  $galaxy_server = db_query($sql, array(':galaxy_id' => $galaxy_id));
  $galaxy_server = $galaxy_server->fetchObject();

  // Obtain the hostname and port from the url
  $matches = array();
  $use_https = FALSE;
  $http_regex = "((https|http):\/\/)?";
  // TODO: this host_regex is dependent on whether the server is
  // something.com or the server's address is simply localhost
  //$host_regex = "([a-z0-9-.]*).([a-z0-9-.]*)";
  $host_regex = "([a-z0-9-.]*)";
  $port_regex = "(:)?([0-9]*)?";
  $regex = $http_regex . $host_regex . $port_regex;
  preg_match("/^$regex$/", $galaxy_server->url, $matches);
  $hostname = $matches[3];
  $port = $matches[5];

  if($matches[2] == "https"){
    $use_https = TRUE;
  }

  // Load the blend4php library into scope and create the Galaxy instance.
  $library = libraries_load('blend4php');
  $galaxy_instance = new GalaxyInstance($hostname, $port, $use_https);

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
  if (!$workflow){
    drupal_set_message('Cannot find workflow. ' . $galaxy_instance->getErrorMessage(), 'error');
    return FALSE;
  }

  // Select a target history by using the first history owned by the user.
  // The history_id is required for retrieving tool information for a workflow.
  // Also, the workflow must be owned by the user.
  $galaxy_history = new GalaxyHistories ($galaxy_instance);

  // Obtain the current history
  $galaxy_current_history = $galaxy_history->getMostRecentlyUsed();

  // Obtain history contents for the current history
  $history_id = $galaxy_current_history['id'];

  // Obtain a galaxy history contents object
  $galaxy_history_contents = new GalaxyHistoryContents ($galaxy_instance);

  // Current history contents is
  $current_history_contents = $galaxy_history_contents->index(array('history_id' => $history_id));

  // The array of components for the webform.
  $components = array ();
  $components[] = array (
    'cid' => 1,
    'pid' => 0,
    'form_key' => 'galaxy_webform',
    'name' => 'galaxy_webform',
    'type' => 'hidden',
    // 'value' => $galaxy_id,
    'extra' => array (
      'private' => 1,
      'hidden_type' => 'value',
      'analysis' => FALSE,
      'wfid' => $workflow_id,
      'hid' => $history_id,
      'host' => $hostname,
      'port' => $port,
      'https' => $use_https,
      'api_key' => $galaxy_server->api_key
    ),
    'required' => '0',
    'weight' => 0
  );

  // The conditionals array for the webform
  $conditionals = array();

  // Construct web form components based on workflow steps.
  $i = 1;
  $steps = $workflow['steps'];

  foreach ($steps as $step_index => $step){

    // Get the tool for this step.  Some steps don't have tools so the tool_id
    // stays as null if there isn't one.
    $tool = NULL;
    // If a given step does the have the key 'tool_id' it will be placed here
    $tool_id = $step['tool_id'];

    // Generate the vanilla tool to then be placed on the form
    if ($tool_id){
      $tool_instance = new GalaxyTools($galaxy_instance);
      $tool = $tool_instance->build(
        array(
        'tool_id' => $tool_id,
        'history_id' => $history_id)
      );

      if (!$tool){
        drupal_set_message('Cannot retrieve tool information from Galaxy: ' . $tool_id, 'Error' );
        return FALSE;
      }
    }

    // Each step is contained in a fieldset. We'll name the field set after
    // it's step and if a tool is present we'll include the tool info.
    $cid = count($components) + 1;
    $fieldset_key = "Step_" . $i . "_fieldset";
    $fieldset_name = 'Step ' . $i;
    $fieldset_desc = '';

    if ($tool){
      $fieldset_name .=  ': ' . $tool['name'] . ' v' .  $tool['version'];
      $fieldset_desc = $tool['description'];
    }

    else if($step['annotation']){
      $fieldset_name .=  ': ' . $step['annotation'];
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
    if ($tool){
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
    if (!$tool){
      tripal_galaxy_add_webform_non_tool_input ( $components, $step, $i, $cid, $current_history_contents );
    }

    // Otherwise, iterate over tool inputs and add webform components.
    else{
      foreach ( $tool['inputs'] as $tool_input){
        tripal_galaxy_add_webform_tool_input ($components, $conditionals,
        		$tool_input, $steps, $i, $cid);
      }
    }
    // Increment the step counter
    $components[] = array(
      'cid' => count($components) + 1,
      'pid' => 0,
      'name' => "Step" . ($step_index + 1),
      'form_key' => "Step_" . ($step_index + 1) ."_pagebreak",
      'type' => 'pagebreak',
      'weight' => count($components) + 1
    );
    $i++;
  }

  // How do I extend the node type so it tells me in the form id that this
  // is a 'galaxy' webform?
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
    'submit_limit' => '-1',
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
function tripal_galaxy_add_webform_non_tool_input(&$components, $step, $step_index, $pid = 0, $history_contents) {

  $items = "";
  // Iterate through all of the history contents, add them as a selectable item
  // Id is the item value and name is the human readable
  foreach ( $history_contents as $content ) {
    $items .= $content['id'] . "|" . $content['name'] . "\r\n";
  }

  $extra = array (
    'title_display' => 'before',
    'description_above' => 1,
    'items' => $items,
    'aslist' => 1
  );

  $cid = count ( $components ) + 1;
  switch ($step['type']) {

    // If this is for a data 'file' input
    case 'data_input' :
      $components[] = array (
        'cid' => $cid,
        'pid' => $pid,
        'name' => $step['tool_inputs']['name'],
        'form_key' => 'dataset_' . $step_index,
        'type' => 'select',
        'value' => "",
        'extra' => $extra,
        'weight' => $cid,
        'required' => 1
      );

      break;
    default :
      break;
  }
}


/**
 * A function to construct a web form component based on a given tool input
 *
 * Some of these workflows will have its 'vanilla' inputs and others will have
 * its inputs already chosen (workflow might have been constructed on the
 * galaxy server or imported)
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
function tripal_galaxy_add_webform_tool_input(&$components, &$conditionals, $tool_input, $steps, $step_index, $pid = 0){
  $def = array();
  foreach($steps[$step_index - 1]['tool_inputs'] as $k => $v){
    $def[$k] = json_decode($v);
  }

  // Initiate variables. By default, these variables will create
  // A drop-down list webform component.
  $is_list = 1;
  $webform_value = "";
  $webform_type = 'select';
  $input_name = '';
  $is_required = 1;
  $extra = array(
    'title_display' => 'before',
    'description_above' => 1,
    'items' => '',
    'aslist' => $is_list,
  );

  // Set the default value accordingly.
  if(array_key_exists('optional', $tool_input) and $tool_input['optional']){
    $is_required = 0;
  }

  if (array_key_exists('help', $tool_input) and $tool_input['help'])
    $extra['description'] = $tool_input['help'];

  // Extract tool input form information to a drupal webform component.
  switch ($tool_input['type'])
  {
    case 'text':
      // Text should be the webform equivalent of textfield.
      $webform_type = 'textfield';
      extract_defaults($def, $tool_input, $webform_value);
      break;

    case 'select':
      // Select should be the default list drop-down. Unless it has a display
      // of Checkbox
      if(array_key_exists('options', $tool_input))
      {
        foreach ($tool_input['options'] as $key => $val)
        {
          // The val is now that sub-array containing the human readable option
          // (index 0) as well as the unique key-name (index 1).
            if($val[1])
              $extra['items'] .= $val[1] . "|" . $val[0] . "\n";
            else
              $extra['items'] .= $step_index . "|" . $val[0] . "\n";
          }
        }
        // Check for default params
        extract_defaults($def, $tool_input, $webform_value );
      break;


      // TODO: Error here
    case 'data_column':
      // Data_column should be the default list drop-down.
      $extra['multiple'] = 0;
      // dpm($tool_input);
      break;

    case 'data':
      // Data appears to be a file selection of files that already
      // exists in the user's history.
      $webform_type = 'select';
      foreach ($tool_input['options'] as $key => $vals)
      {
        if(empty($vals)){
          $extra['items'] .=  "na|N\A\n";
        }

        else{
          foreach ($vals as $details){
              $extra['items'] .= $details['id'] . "|" . $details['name'] . "\n";
          }
        }
      }
      // $webform_value = '';
      break;

    case 'data_collection':
      $webform_type = 'select';

      foreach ($tool_input['options'] as $key => $vals){
        if(empty($vals))
          $extra['items'] .=  "na|N\A\n";
        else{
          foreach ($vals as $details){
              $extra['items'] .= $details['id'] . "|" . $details['name'] . "\n";
          }
        }

      }
      break;

    case 'integer':
    	$webform_type = 'number';
      extract_defaults($def, $tool_input, $webform_value );
      break;

    case 'float':
    $webform_type = 'number';
    extract_defaults($def, $tool_input, $webform_value );
      break;

    case 'boolean':
      // Boolean should have a single true/false value. It is also likely to
      // not have 'options' so we must invent our own (galaxy uses 'no' as The
      // false value and 'yes' as the true value)
      $extra['items'] = "yes|Yes\nno|No";
      $extra['aslist'] = 0;

      // Check defaults
      extract_defaults($def, $tool_input, $webform_value );
      break;

    case 'hidden_data':
      break;

    case 'repeat':
    // This case is important for tools that can have multiple operations
    // being executed within the particular tool (this type is found in the
    // trimmomatic)
    // TODO: Finish this
      break;

    case 'conditional':
      // A conditional case contains nested components but those components
      // should only be shown when specific selections are made in other
      // components. We will recursively add all the components and then
      // create the conditional rules that webforms will use to show the
      // conditional fields.

      tripal_galaxy_add_webform_tool_input($components, $conditionals,
                                           $tool_input['test_param'], $steps,
                                           $step_index, $pid);

      $num_components = count($components);
      // This source_id is referring to the options that are found in the test
      // param
      $source_id = $components[$num_components - 1]['cid'];

      // // Add the case paramters.
      foreach ($tool_input['cases'] as $case_num => $case){
        $conditional = array(
          'andor' => NULL,
          'rules' => array(
            array(
              'source_type' => 'component',
              // The conditional's parent
              'source' => $source_id,
              // This means the element will be listening for the 'value'
              // (see the next element in this array)
              'operator' => 'equal',
              // This is the option for which this conditional is activated.
              'value' => $case['value'],
            ),
          ),
          'actions' => array()
        );

        if($case['inputs']){

          $component_conditionals = array();

          // Iterate through the cases and add a new component and a new
          // conditional action.
          foreach ($case['inputs'] as $cindex => $input){
            // The index for the target component is the very next component.
            // Since the components array is zero-based we can just get the
            // total components and that becomes our index.
            $target_component_index = count($components);


            tripal_galaxy_add_webform_tool_input($components,
                                                 $component_conditionals,
                                                 $input, $steps, $step_index,
                                                 $pid);


            $target_id = $components[$target_component_index]['cid'];

            $conditional['actions'][] = array(
              'target_type' => 'component',
              'target' => $target_id,
              'action' => 'show',
              //'argument' => '',
            );
          }
          // In order to get the conditionals in the proper order, we need to
          // add those that are higher up in the tree first, and those that
          // occur lower in the tree later. To make this happen we save the
          // recursive conditionals in the $sub_conditions array and then add
          // those after we add the current conditional.
          $conditionals[] = $conditional;

          foreach ($component_conditionals as $component_conditional){
            $conditionals[] = $component_conditional;
          }
        }

      }

      // We won't add a 'conditional' as a component so we can short circuit
      // this function and return.
      return;
      break;

    default :
      drupal_set_message('Unhandled tool input type: ' . $tool_input['type'],
      'error');
      break;
  }

  // Get the input name.
  $input_name = NULL;
  if (array_key_exists('label', $tool_input))
    $input_name = $tool_input['label'];
  else if(array_key_exists('title', $tool_input))
    $input_name = $tool_input['title'];

  // If this field recieves it's value from a previous step then we want
  // to set the type as markup and print that information
  $tool_name = $tool_input['name'];


  // TODO: There a is a problem here  in the defualt generation where Output
  // steps are not being associated
  if (array_key_exists($tool_name, $steps[$step_index-1]['input_steps'])){
    $source_step = $steps[$step_index-1]['input_steps'][$tool_name]['source_step'];
    $step_output = $steps[$step_index-1]['input_steps'][$tool_name]['step_output'];
    $webform_type = 'markup';
    $extra['format'] = 'full_html';
    $webform_value =  "<label>" . $input_name . "</label>" .
      "Output from step " . ($source_step + 1);
  }

  $extra['param_name'] = $tool_input['name'];

  $cid = count($components) + 1;
  $components[] = array(
    'cid' => $cid,
    'pid' => $pid,
    'name' => $input_name,
    // This form_key has to be unique from ANYTHING else, otherwise the
    // component will not be presented
    'form_key' => "Step_" . $step_index ."_" . $tool_name . "_" . $cid,
    'type' => $webform_type,
    'value' => $webform_value,
    'extra' => $extra,
    'weight' => $cid,
    'required' => $is_required,
  );

}

/*
* Unpack a default array from the steps of a given workflow
*
* Due to the recursive nature of the conditionals in generating this webform
* there must a function that also recursively renders each component's defaults
*
* This is being designed for the 'select' tool_type first
*
* $default_obj: The defualt array that was acquired from a particular step
*               within the workflow
*
* $tool_input: The tool key to search for inside of the defaults to
*                  associate the
*
* This is to be used in the switch statement cases while determining what
* defualt value to add to the specific component of a tool_input
*
* Returns: The particular value of the component (if it exists) to use
*
*/
function extract_defaults($default_obj, $tool_input, &$webform_value){

// This will be recursive
// Base case will be if the current val of the key => val pair IF the val is
// not an object or array
  foreach($default_obj as $key => $val){
    if(is_object($val)){
      extract_defaults($val, $tool_input, $webform_value);
    }
    switch($tool_input['type']){
      case 'integer':
        if($tool_input['name'] == $key){
          $webform_value = (int)$val;
        }
        break;
      case 'float':
      if($tool_input['name'] == $key){
        $webform_value = (float)$val;
      }
        break;
      case 'text':
      if($tool_input['name'] == $key){
        $webform_value = $val;
      }
        break;
      case 'select':
        if($tool_input['name'] == $key){
          $webform_value = $val;
        }
        break;
      case 'data':
        // return;
        break;
      case 'boolean':
      if($tool_input['name'] == $key){
        if($val == 'true'){
          $webform_value = 'yes';
        }
        else{
          $webform_value = 'no';
        }
    }
        break;
      case 'data_collection':
      // return;
        break;
      case 'repeat':
      // return;
        break;
      default:
        break;
    }

    // dpm($val);
  }



  // If we're out here then we have decompressed such that the val is not
  // another nested object to access so now we can do the iterations
}
