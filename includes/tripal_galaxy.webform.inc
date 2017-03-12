<?php

/**
 * The different tool input parameters:
 * https://github.com/galaxyproject/galaxy/blob/dev/client/galaxy/scripts/mvc/form/form-parameters.js#L96
 */

/**
 * The base function to submitted_list()
 */
function tripal_galaxy_build_webform($galaxy_id, $workflow_id, &$nid) {

  // An object that will contain the componentns and conditions that
  // will be used to build the webform.
  $webform = new stdClass();
  $webform->components = array();
  $webform->conditionals = array();

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
  dpm($workflow);

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
  $webform->components[] = array (
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

  // Construct web form components based on workflow steps.
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
    $cid = count($webform->components) + 1;
    $fieldset_key = "Step_" . $step_index . "_fieldset";
    $fieldset_name = 'Step ' . ($step_index + 1);
    $fieldset_desc = '';
    if ($tool){
      $fieldset_name .=  ': ' . $tool['name'] . ' v' .  $tool['version'];
      $fieldset_desc = $tool['description'];
    }
    else if($step['annotation']){
      $fieldset_name .=  ': ' . $step['annotation'];
    }

    // Add the fieldset to the components list.
    $webform->components[] = array(
      'cid' => $cid,
      'pid' => '0',
      'form_key' => $fieldset_key,
      'name' => $fieldset_name,
      'type' => 'fieldset',
      'value' => '',
      'extra' => array(
        'description' => t('Please provide values for any fields that
            appear below.  If no fields appear you may skip to the next
            step. If an "Additional Settings" section is present it will
            cotnain fields that already have defaults. You may change those
            if desired.'),
        'description_above' => 1,
        'private' => 0,
        'css_classes' => '',
        'title_display' => 0,
        'collapsible' => 1,
        'collapsed' => 0,
        // 'description' => $fieldset_desc,
      ),
      'required' => '0',
      'weight' => $cid,
    );

    // If this step isn't a tool then handle it appropriately.
    if (!$tool){
      tripal_galaxy_build_webform_add_non_tool_input($webform, $step, $step_index, $cid, $current_history_contents );
    }
    // Otherwise, handle the tool.
    else {
      dpm($tool);
      $dcid = count($webform->components) + 1;
      $webform->components[] = array(
        'cid' => $dcid,
        'pid' => $cid,
        'form_key' => $fieldset_key . '_additional',
        'name' => 'Additional Settings',
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
        'weight' => '100',
      );
      $webform->current_optional_fieldset = $dcid;

//       $ddcid = count($webform->components) + 1;
//       $webform->components[] = array(
//         'cid' => $ddcid,
//         'pid' => $dcid,
//         'form_key' => $fieldset_key . '_defaults_details',
//         'name' => $tool['name'] . ' defaults',
//         'type' => 'markup',
//         'value' => '',
//         'extra' => array(
//           'format' => 'full_html',
//           'private' => 0,
//           'css_classes' => '',
//           'title_display' => 0,
//         ),
//         'required' => '0',
//         'weight' =>  $dcid,
//       );

      //
      //$dpid = $ddcid;

      foreach ($tool['inputs'] as $input) {
        $step = $workflow['steps'][$step_index];
        tripal_galaxy_build_webform_add_tool_input($webform, $workflow,
          $step, $tool, $input, $cid, FALSE);
      }
    }

    // Add a page break after each step.
    $webform->components[] = array(
      'cid' => count($webform->components) + 1,
      'pid' => 0,
      'name' => "Step" . ($step_index + 1),
      'form_key' => "Step_" . ($step_index + 1) ."_pagebreak",
      'type' => 'pagebreak',
      'weight' => count($webform->components) + 1
    );
  }

  dpm($webform);

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
    'teaser' => '1',
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
    'progressbar_page_number' => '1',
    'progressbar_percent' => '1',
    'progressbar_pagebreak_labels' => '1',
    'progressbar_include_confirmation' => '1',
    'progressbar_label_first' => 'Step 0',
    'progressbar_label_confirmation' => 'Submit',
    'preview' => '1',
    'preview_next_button_label' => 'Next Step',
    'preview_prev_button_label' => 'Previous Step',
    'preview_title' => '',
    'preview_message' => '',
    'preview_message_format' => NULL,
    'record_exists' => TRUE,
    'roles' => array (
     '1', '2',
    ),
    'emails' => array(),
    'components' => $webform->components,
    'conditionals' => $webform->conditionals
  );

  // Save the node.
  node_save($node);
  // A new node will have been created be sure to record the nid in the
  // workflow table on the db
  $nid = (int)$node->nid;
  return TRUE;
}

/**
 *
 */
function tripal_galaxy_build_webform_add_non_tool_input($webform, $step,
  $step_index, $pid = 0, $history_contents) {

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

  $cid = count ( $webform->components ) + 1;
  switch ($step['type']) {

    // If this is for a data 'file' input
    case 'data_input' :
      $webform->components[] = array (
        'cid' => $cid,
        'pid' => $pid,
        'name' => $step['tool_inputs']['name'],
        'form_key' => 'dataset_' . $step_index,
        'type' => 'BDSS_file',
        // 'value' => "",
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
 * @param $webform
 *   An object containinng the components and conditionals into which the
 *   webform will be created.

 *
 * @return
 *   An array suitable for the 'components' webform array
 *   False otherwise.
 */
function tripal_galaxy_build_webform_add_tool_input($webform, $workflow,
    $step, $tool, $tool_input, $pid = 0) {

  $step_index = $step['id'];

  // Some tool inputs are nested. These include conditionals, repeats and
  // operators.  Therefore we weant to check what kind of input we
  // are dealing with.  If it's not one of those nested components then
  // we can add the component. Otherwise we have to deal with each one
  // specially.
  switch ($tool_input['type']) {


    case 'repeat':
      // This case is important for tools that can have multiple operations
      // being executed within the particular tool (this type is found in the
      // trimmomatic)
      // TODO: Finish this
      break;


    case 'conditional':
      // Each conditional has a test param component.  So, let's add that first.
      $test_param = $tool_input['test_param'];
      $value = '';
      $input_values = $workflow['steps'][$step_index]['tool_inputs'];
      dpm($tool_input['name']);
      dpm($test_param);
      if (array_key_exists($tool_input['name'], $input_values)) {
        $input_values = json_decode($input_values[$tool_input['name']], TRUE);
        dpm($input_values);
        if (array_key_exists($test_param['name'], $input_values)) {
          $value = $input_values[$test_param['name']];
        }
      }
      tripal_galaxy_build_webform_add_component($webform, $workflow, $step,
          $tool, $test_param, $pid, $value, TRUE);

      // Each condition must have a "source" component that they refer too.
      // Our source component is the test param.
      $test_param_index = count($webform->components);
      $source_id = $webform->components[$test_param_index - 1]['cid'];

      // TODO: Whatis this for???
      if((int)$source_id == 1){
        return;
      }

      // Each valid value of the test parameter (i.e. cases) will lead to
      // a different set of inputs that will be made available.  We need
      // to add all of those input components and associate them with the
      // correct case for the test param.
      foreach ($tool_input['cases'] as $case_num => $case) {
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

        // If this case has input components we need to add them.
        if ($case['inputs']){

          // Iterate through the case inputs and add a new component for each
          // one and a new conditional action.
          foreach ($case['inputs'] as $cindex => $case_input){
            // The index for the target component is the very next component.
            // Since the components array is zero-based we can just get the
            // total components and that becomes our index.
            $target_component_index = count($webform->components);
            $value = '';
            $input_values = $workflow['steps'][$step_index]['tool_inputs'];
            if (array_key_exists($tool_input['name'], $input_values)) {
              $input_values = json_decode($input_values[$tool_input['name']], TRUE);
              if (array_key_exists($case_input['name'], $input_values)) {
                $value = $input_values[$case_input['name']];
              }
            }
            tripal_galaxy_build_webform_add_component($webform, $workflow, $step,
                $tool, $case_input, $pid, $value, FALSE);
            $target_id = $webform->components[$target_component_index]['cid'];

            // For each input we'll add an action to show the field.
            $conditional['actions'][] = array(
              'target_type' => 'component',
              'target' => $target_id,
              'action' => 'show',
            );
          }
          $webform->conditionals[] = $conditional;
        }
      }
      // We won't add a 'conditional' as a component so we can short circuit
      // this function and return.
      return;
      break;

    // If we don't have one of the nested types, then we have an input
    // type that is a single component, so let's add it.
    default:
      $input_values = $workflow['steps'][$step_index]['tool_inputs'];
      $value = array_key_exists($tool_input['name'], $input_values) ? $input_values[$tool_input['name']] : '';
      tripal_galaxy_build_webform_add_component($webform, $workflow, $step,
          $tool, $tool_input, $pid, $value);
      break;
  }
}

/**
 *
 * @param $webform
 * @param $step
 * @param $input
 * @param $pid
 * @param $value
 * @param $fixed
 */
function tripal_galaxy_build_webform_add_component($webform,
    $workflow, $step, $tool, $input, $pid, $value, $fixed = FALSE) {

  // Initiate variables that will be used for building the webform component.
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
    'param_name' => $input['name'],
  );
  $webform_value = $value;
  $tool_name = $input['name'];


  // Is this a required component?
  if(array_key_exists('optional', $input) and $input['optional']){
    $is_required = 0;
  }

  // Get the description for this component.
  if (array_key_exists('help', $input)){
    $extra['description'] = ucfirst($input['help']);
  }

  // Get the input name.
  $input_name = NULL;
  if (array_key_exists('label', $input)) {
    $input_name = $input['label'];
  }
  else if(array_key_exists('title', $input)) {
    $input_name = $input['title'];
  }

  // Now, set the webform component type and the value depending on the
  // tool input type.
  switch ($input['type']) {
    case 'text':
      // Text should be the webform equivalent of textfield.
      $webform_type = 'textfield';
      break;

    case 'select':
      $webform_type = 'select';
      // Select should be the default list drop-down. Unless it has a display
      // of Checkbox
      if(array_key_exists('options', $input)) {
        foreach ($input['options'] as $key => $val) {
          // The val is now that sub-array containing the human readable option
          // (index 0) as well as the unique key-name (index 1).
          if($val[1])
            $extra['items'] .= $val[1] . "|" . $val[0] . "\n";
          else
            $extra['items'] .= $step_index . "|" . $val[0] . "\n";
        }
      }
      break;

    case 'data_column':
      $webform_type = 'select';
      // Data_column should be the default list drop-down.
      $extra['multiple'] = 0;
      break;

    case 'data':
      // Data appears to be a file selection of files that already
      // exists in the user's history.
      $webform_type = 'BDSS_file';
      break;

    case 'data_collection':
      $webform_type = 'select';

      foreach ($input['options'] as $key => $vals){
        if(empty($vals)){
          $extra['items'] .=  "na|Not Available\n";
        }
        else{
          foreach ($vals as $details){
            $extra['items'] .= $details['id'] . "|" . $details['name'] . "\n";
          }
        }
      }
      break;

    case 'integer':
      $webform_type = 'number';
      $webform_value = (int) $value;
      break;

    case 'float':
      $webform_type = 'number';
      $webform_value = (float) $value;
      break;

    case 'boolean':
      $webform_type = 'select';
      if($value == 'true'){
        $webform_value = 'yes';
      }
      else{
        $webform_value = 'no';
      }
      // Boolean should have a single true/false value. It is also likely to
      // not have 'options' so we must invent our own (galaxy uses 'no' as The
      // false value and 'yes' as the true value)
      $extra['items'] = "yes|Yes\nno|No";
      $extra['aslist'] = 0;

      // Check defaults
      break;

    // It's possible for multiple conditionals to be nested inside
    // each other.  We don't create a component for a conditional, but
    // we need to do some recursion so we can handle it.
    case 'conditional':
      tripal_galaxy_build_webform_add_tool_input($webform, $workflow,
          $step, $tool, $input, $pid);
      return;
      break;

    case 'hidden_data':
      break;

    default :
      drupal_set_message('Unhandled tool input type: ' . $input['type'] . ".", 'error');
      break;
  }

  // If this is a test param then automatically change the webform type to
  // a fixed value. This is because a test param was set by the person
  // who created the workflow and should not be changed by the user.
  if ($fixed) {
    $webform_type = 'fixed_value';
  }

  // If the input step uses an output from another tool then we need
  // to set that and not let the user change it.
  if (array_key_exists($tool_name, $step['input_steps'])){
    $source_step = $step['input_steps'][$tool_name]['source_step'];
    $step_output = $step['input_steps'][$tool_name]['step_output'];
    $webform_type = 'hidden';
    $webform_value =  "Output from step " . ($source_step + 1);
  }

  // If this component has a value then put it in the "Additional Settings"
  // fielset.
  if ($webform_value) {
    $pid = $webform->current_optional_fieldset;
  }

  $cid = count($webform->components) + 1;
  $webform->components[] = array(
    'cid' => $cid,
    'pid' => $pid,
    'name' => $input_name,
    // This form_key has to be unique from ANYTHING else, otherwise the
    // component will not be presented
    'form_key' => "Step_" . $step['id'] . "_" . $tool_name . "_" . $cid,
    'type' => $webform_type,
    'value' => $webform_value,
    'extra' => $extra,
    'weight' => $cid,
    'required' => $is_required,
  );
}

/**
 * This is for tools that have their input files determined by the output of a
 * preceding tool.
 *
 *
 */
function tripal_galaxy_build_webform_get_input_steps($default_files, $tool_input,
    &$webform_value, &$webform_type, &$extra){
  // The tool input type will be select, if there is a key in the $default_files
  // that matches the $tool_input name then we change the webform_type to
  // markup and the value to the name of the step's output that this
  // particular input file is taking in
  foreach($default_files as $key => $val){
    // Again $val will be an array containing the elements
    // step_output AND (more importantly) source_step
  }
}

/**
 * Unpack a default array from the steps of a given workflow.
 *
 * @param $default_obj
 *
 * @return
 *   The default value of the component (if it exists).
 *
 */
function tripal_galaxy_build_webform_get_step_defaults($defaults, $tool_input,
    &$webform_value) {

  // Iterate through all of the defaults for this step.
  foreach ($defaults as $key => $val) {

    // If the value is an object then we want to recurse.
    if(is_object($val)){
      tripal_galaxy_build_webform_get_step_defaults($val, $tool_input, $webform_value);
    }

    // If thie default is not for this input then skip it.
    if($tool_input['name'] != $key){
      continue;
    }

    // Depending on the input type, then the weborm element type will
    // be different.
    switch($tool_input['type']){
      case 'integer':
        $webform_value = (int) $val;
        break;
      case 'float':
        $webform_value = (float)$val;
        break;
      case 'text':
        $webform_value = $val;
        break;
      case 'select':
        $webform_value = $val;
        break;
      case 'data':
        // This is for tripal_galaxy_build_webform_get_input_steps
        break;
      case 'boolean':
        if($val == 'true'){
          $webform_value = 'yes';
        }
        else{
          $webform_value = 'no';
        }
        break;
      case 'data_collection':
        break;
      case 'repeat':
        break;
      default:
        break;
    }
  }
}
