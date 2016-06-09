<?php

/**
 * The different tool input parameters: 
 * https://github.com/galaxyproject/galaxy/blob/dev/client/galaxy/scripts/mvc/form/form-parameters.js#L96
 */



/**
 * The base function to submitted_list()
 */
function tripal_galaxy_admin_submitted_list_base($galaxy_id, $workflow_id) {
  // Set up basic node defaults.
  if ($workflow_id == NULL) {
    drupal_set_message ( t ( 'This page currently not supported' ), 'error' );
  }
  
  // Obtain the galaxy server from the galaxy id
  $galaxy_server = db_query ( "SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id", array (
    ':galaxy_id' => $galaxy_id 
  ) );
  $galaxy_server = $galaxy_server->fetchObject ();
  
  dpm ( "This is the galaxy server: " );
  dpm ( $galaxy_server );
  
  // Obtain the hostname and port from the url
  $matches = array ();
  $regex = "((https|http):\/\/)?"; // SCHEME
  $regex .= "([a-z0-9-.]*)"; // Host or IP
  $regex .= "(:)?([0-9]*)?"; // Port
  preg_match ( "/^$regex$/", $galaxy_server->url, $matches );
  $hostname = $matches[3];
  $port = $matches[5];
  
  // Set up the Galaxy instance
  $library = libraries_load ( 'GalaxyLib-PHP' );
  
  $galaxy_instance = new GalaxyInstance ( $hostname, $port );
  $galaxy_instance->setAPIKey ( $galaxy_server->api_key );
  
  // Set up the workflow instance
  $workflow_instance = new GalaxyWorkflows ( $galaxy_instance );
  $workflow = $workflow_instance->show ( $workflow_id );
  
  // Select a target history by finding a history owned by the current user
  $history_instance = new GalaxyHistories ( $galaxy_instance );
  $histories = $history_instance->index ();
  $target_history = $histories[0];
  
  dpm ( "This is the Galaxy instance:" );
  dpm ( $galaxy_instance );
  
  dpm ( "This is the workflow id: " );
  dpm ( $workflow_id );
  
  dpm ( "This is the workflow instance: " );
  dpm ( $workflow_instance );
  
  dpm ( "This is the workflow:" );
  dpm ( $workflow );
  
  dpm ( "This is the history" );
  dpm ( $histories );
  
  // Default values for the node
  $node = new stdClass ();
  $node->type = 'webform';
  node_object_prepare ( $node );
  $node->title = $workflow['name'];
  $node->language = 'en';
  $node->body[LANGUAGE_NONE][0]['value'] = '';
  $node->body[LANGUAGE_NONE][0]['format'] = 'full_html';
  $node->uid = 1;
  $node->promote = 0;
  $node->comment = 0;
  
  
  $components = NULL;
  
  // Make sure we have valid input
  if ($workflow == NULL || ! is_array ( $workflow ) || ! array_key_exists ( 'steps', $workflow )) {
    return;
  }
  
  // Construct web form components based on workflow steps.
  $i = count ( $workflow['steps'] );
  foreach ( array_reverse ( $workflow['steps'] ) as $step ) {
    
    dpm ( "The step: " );
    dpm ( $step );
    
    $components[$i] = array (
      'name' => 'Step ' . ($i),
      'form_key' => 'the_step_' . ($i),
      'type' => 'markup',
      'weight' => 26,
      'value' => 'Step ' . ($i),
      'mandatory' => 0,
      'pid' => 0 
    );
    
    // If the step is of type data_input. This step is to prompt the user for
    // a file on their computer. Note this is not a tool.
    if ($step['type'] == "data_input") {
      dpm ( "I am a file upload! " );
      
      $components[$i] = array (
        'name' => 'Upload dataset',
        'form_key' => 'dataset_' . $i,
        'type' => 'file',
        'weight' => 26,
        'mandatory' => 1,
        'pid' => 0 
      );
      $i --;
      continue;
    }     

    // If the workflow step is not a data upload, figure out what it needs
    else { 
      
      // Get the tool_id, continue if it is null. All steps should have
      // exactly one Tool.
      $tool_id = $step['tool_id'];
      if ($tool_id == NULL) {
        $i ++;
        continue;
      }
      
      $tool_instance = new GalaxyTools ( $galaxy_instance );
      $tool = $tool_instance->build ( array (
        'tool_id' => $tool_id,
        'history_id' => $target_history['id'] 
      ));
      dpm ( "This is the Tool:" );
      dpm ( $tool_id );
      dpm ( $tool );
      
      if($tool == FALSE || $tool == NULL){
        drupal_set_message('Error rendering workflow information.' , 'Error');
        return;
      }
      
      // Iterate over tool inputs
      foreach ( $tool["inputs"] as $input ) {
        
        dpm ( "This is the input: " );
        dpm ( $input );
               
        // If there is no type in this input, skip this input
        if(!array_key_exists('type', $input)){
          continue;
        }
        
        // Add The appropriate webform component to 
        // to match each of the input models
        $component = construct_component($input);
        if($component != FALSE){
          $components[] = $component;
        }
      }
    }
    
    // Decrement the counter
    $i --;
  }
  
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
    'submit_text' => '',
    'submit_limit' => '-1', // User can submit more than once.
    'submit_interval' => '-1',
    'total_submit_limit' => '-1',
    'total_submit_interval' => '-1',
    'record_exists' => TRUE,
    'roles' => array (
      0 => '1' 
    ),

    'components' => $components 
  );
  
  // Save the node.
  node_save ( $node );
}

/**
 * A function to construct a web form component based on 
 *   A given tool input.
 * 
 * @param $tool_input 
 *   The tool input to build the component for. Must have Key 'type'.
 * 
 *  @param $index
 *    The index to help set the tool name to.
 *  
 * @return 
 *   An array suitable for the 'components' webform array 
 *   False otherwise. 
 */
function construct_component($tool_input, $index) {
 
  // Initiate variables. By default, these variables will create
  // A drop-down list webform component.
  $options = array();
  $component = array ();
  $is_list = 1;
  $has_multiple = 0;
  $mandatory = 0;
  $webform_items = "";
  $webform_value = "";
  $webform_type = 'select';
  
  if (array_key_exists ( 'options', $tool_input )) {
    $options = $tool_input['options'];
  }
  
  // Add options to the webform items list.
  $x = 0;
  foreach ( $options as $option ) {
    if (count ( $option ) == 0) {
      continue;
    }
    // Items must be in the form 'key1|value1 \n key2|value2'; 
    $webform_items .= $option[0] ."_" . $x . "|" . $option[0] ."\n";
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
        $is_list = 0;
        $has_multiple = 1;
      }
      
      break;
    
    case 'data_column':
      
      // Data_column should be the default list drop-down.
      break;
    
    case 'genomebuild':
      
      break;
    
    case 'data':
      
      // For the most part, this should be the default drop down list webform
      // Component.
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
      $is_list = 0;
      $webform_items = "t|True";
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
    
    default :
      
      break;
  }
    
   // Obtain the tool name.
  $input_name = NULL;
  if (! array_key_exists ( 'label', $tool_input )) {
    $input_name = $tool_input['title'];
  } else {
    $input_name = $tool_input['label'];
  }
  
  // The 'extra' field in the webform component must be
  // based on the webform type.
  if ($webform_type == 'textfield') {
    $extra = array (
      'title_display' => 'inline',
      'private' => 0 
    );
  } else {
    $extra = array (
      'title_display' => 'inline',
      'private' => 0,
      'items' => $webform_items,
      'aslist' => $is_list,
      'multiple' => $has_multiple 
    );
  }
  
  // Default component. Defaults to a drop-down list component.
  $component = array (
    'name' => $input_name . "_" . $index,
    'form_key' => $input_name . "_" . $index,
    'type' => $webform_type,
    'value' => $webform_value,
    'mandatory' => $mandatory,
    'weight' => 30,
    'pid' => 0,
    'extra' => $extra,
  );
  
  return $component;
}


/**
 * A funciton for building the workflow's input form
 */
function tripal_galaxy_admin_submitted_list() {
  tripal_galaxy_admin_submitted_list_base ( 0 );
}


function test_webforms() {
  $node = new stdClass ();
  $node->type = 'webform';
  node_object_prepare ( $node );
  $node->title = 'Contact Us';
  $node->language = 'en';
  $node->body[LANGUAGE_NONE][0]['value'] = '';
  $node->body[LANGUAGE_NONE][0]['format'] = 'full_html';
  $node->uid = 1;
  $node->promote = 0;
  $node->comment = 0;
  
  // Create the webform components.
  $components = array (
    array (
      'name' => 'Gender',
      'form_key' => 'gender',
      'type' => 'select',
      'mandatory' => 1,
      'weight' => 0,
      'pid' => 0,
      'extra' => array (
        'title_display' => 'inline',
        'private' => 0,
        'items' => "Mrs|Mrs\nMiss|Miss\nMr|Mr",
        'aslist' => 1 
      ) 
    ),
    array (
      'name' => 'Last name',
      'form_key' => 'name',
      'type' => 'textfield',
      'mandatory' => 1,
      'weight' => 5,
      'pid' => 0,
      'extra' => array (
        'title_display' => 'inline',
        'private' => 0 
      ) 
    ),
    array (
      'name' => 'First name',
      'form_key' => 'first_name',
      'type' => 'textfield',
      'mandatory' => 1,
      'weight' => 10,
      'pid' => 0,
      'extra' => array (
        'title_display' => 'inline',
        'private' => 0 
      ) 
    ),
    array (
      'name' => 'City',
      'form_key' => 'city',
      'type' => 'textfield',
      'mandatory' => 0,
      'weight' => 15,
      'pid' => 0,
      'extra' => array (
        'title_display' => 'inline',
        'private' => 0 
      ) 
    ),
    array (
      'name' => 'Country',
      'form_key' => 'country',
      'type' => 'select',
      'mandatory' => 0,
      'weight' => 20,
      'pid' => 0,
      'extra' => array (
        'title_display' => 'inline',
        'private' => 0,
        'aslist' => 1,
        'options_source' => 'countries' 
      ) 
    ),
    
    array (
      'name' => 'Email address',
      'form_key' => 'email_address',
      'type' => 'email',
      'mandatory' => 1,
      'weight' => 25,
      'pid' => 0,
      'extra' => array (
        'title_display' => 'inline',
        'private' => 0 
      ) 
    ),
    array (
      'name' => 'Subject',
      'form_key' => 'subject',
      'type' => 'select',
      'mandatory' => 1,
      'weight' => 30,
      'pid' => 0,
      'extra' => array (
        'title_display' => 'inline',
        'private' => 0,
        'items' => "s1|Subject 1\nother|Other\nwee|blah",
        'aslist' => 0,
        'multiple' => 1 
      ) 
    ),
    array (
      'name' => 'Message',
      'form_key' => 'message',
      'type' => 'textarea',
      'mandatory' => 1,
      'weight' => 35,
      'pid' => 0,
      'extra' => array (
        'title_display' => 'inline',
        'private' => 0 
      ) 
    ),
    array (
      'name' => 'Mandatory Fields',
      'form_key' => 'mandatory_fields',
      'type' => 'markup',
      'mandatory' => 0,
      'weight' => 40,
      'pid' => 0,
      'value' => 'Fields with * are mandatory',
      'extra' => array (
        'title_display' => 'inline',
        'private' => 0,
        'format' => 'full_html' 
      ) 
    ) 
  );
  
  // Setup notification email.
  $emails = array (
    array (
      'email' => 'somebody@example.tld',
      'subject' => 'default',
      'from_name' => 'default',
      'from_address' => 'default',
      'template' => 'default',
      'excluded_components' => array () 
    ) 
  );
  
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
    'submit_text' => '',
    'submit_limit' => '-1', // User can submit more than once.
    'submit_interval' => '-1',
    'total_submit_limit' => '-1',
    'total_submit_interval' => '-1',
    'record_exists' => TRUE,
    'roles' => array (
      0 => '1' 
    ) // Anonymous user can submit this webform.
,
    'emails' => $emails,
    'components' => $components 
  );
  
  // Save the node.
  node_save ( $node );
}


?>