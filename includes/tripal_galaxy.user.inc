<?php

/**
 * The different tool input parameters: 
 * https://github.com/galaxyproject/galaxy/blob/dev/client/galaxy/scripts/mvc/form/form-parameters.js#L96
 */



/**
 * The base function to submitted_list()
 */
function tripal_galaxy_admin_submitted_list_base($galaxy_id, $workflow_id) {
  
  // Initialize variables. All of these variables are necessary 
  // to create paginated webforms.
  $webform_page = 1;
  $webform_weight = 0;
  $webform_cid = 1;
  $components = NULL;
  
  // Obtain the galaxy server from the galaxy id
  $galaxy_server = db_query ( "SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id", array (
    ':galaxy_id' => $galaxy_id 
  ));
  $galaxy_server = $galaxy_server->fetchObject ();
   
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
  
  // Check to make sure we have a valid connection.
  if($galaxy_instance->getVersion() == NULL){
    drupal_set_message ( t('Cannot connect to galaxy instance'),'error');
    return;
  }
  
  $galaxy_instance->setAPIKey ( $galaxy_server->api_key );
  
  // Set up the workflow instance.
  $workflow_instance = new GalaxyWorkflows ( $galaxy_instance );
  $workflow = $workflow_instance->show ( $workflow_id );
  
  // Select a target history by finding a history owned by the current user.
  $history_instance = new GalaxyHistories ( $galaxy_instance );
  $histories = $history_instance->index ();
  $target_history = $histories[0];
     
  // Make sure we have a valid workflow.
  if ($workflow == NULL || ! is_array ( $workflow ) || ! array_key_exists ( 'steps', $workflow )) {
    drupal_set_message ( t ( 'Unrecognized response from Galaxy server' ), 'error' );
    return;
  }
    
    // Construct web form components based on workflow steps.
  $i = 1;
  foreach ( $workflow['steps'] as $step ) {
    
    // If the step is of type data_input. This step is to prompt the user for
    // a file on their computer. Note this is not a tool.
    if ($step['type'] == "data_input") {
      $components[] = array (
        'cid' => $webform_cid,
        'name' => 'Upload dataset',
        'form_key' => 'dataset_' . $i,
        'type' => 'file',
        'mandatory' => 1,
        'weight' => $webform_weight,
        'pid' => 0,
        'filtering' => array (
          'types' => array (
            'bed',
            'bam',
            'blast' 
          ),
          'addextensions' => '',
          'size' => file_upload_max_size () 
        ),
        'page_num' => $webform_page 
      );
      $webform_weight ++;
      $webform_cid ++;

    }     

    // If the step is not of type data_type. It must be a tool.
    else {
      
      // Get the tool_id, continue if it is null. All steps should have
      // exactly zero or one Tool.
      $tool_id = $step['tool_id'];
      if ($tool_id == NULL) {
        dpm ( "@@@@ THIS TOOL ID IS NULL @@@@" );
        continue;
      }
      
      // Construct the component for this step by fining input information
      // about its tool.
      $tool_instance = new GalaxyTools ( $galaxy_instance );
      $tool = $tool_instance->build ( array (
        'tool_id' => urlencode ( $tool_id ),
        'history_id' => $target_history['id'] 
      ) );
      
      // Make sure tool informaiton is correct
      if ($tool == FALSE || $tool == NULL) {
        dpm ( $galaxy_instance->getErrorMessage () );
        drupal_set_message ( 'Error rendering workflow information.', 'Error' );
        return;
      }
      
      // Iterate over tool inputs
      foreach ( $tool["inputs"] as $input ) {
        
        // If there is no type in this input, skip this input
        if (! array_key_exists ( 'type', $input )) {
          dpm ( "@@@ @@@ @@@ Tool input contains no type! Tool information: @@@ @@@ @@@" );
          dpm ( $tool );
          continue;
        }
        
        // Add The appropriate webform components to
        // to match each of the input models
        $component = construct_component ( $input, $i, $webform_page, $webform_weight, $webform_cid );
        if ($component != FALSE) {
          $components[] = $component;
        }else {
          dpm($component);
        }
      }
    }
    
    // Add a page breaker for the next step.
    $webform_page ++;
    $component[] = array (
      'cid' => $webform_cid,
      'pid' => '0',
      'form_key' => 'pagebreak_' . $i,
      'name' => 'pagebreak_' . $i,
      'type' => 'pagebreak',
      'weight' => $webform_weight,
      'value' => '',
      'extra' => array (
        'private' => 0,
        'next_page_label' => '',
        'prev_page_label' => '' 
      ),
      'required' => '0',
      'page_num' => $webform_page 
    );
    $webform_weight ++;
    $webform_cid ++;
    
    // Increment the counter
    $i ++;
  }
  
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
function construct_component($tool_input, $index, &$webform_page, &$webform_weight, &$webform_cid) {
 
  // Initiate variables. By default, these variables will create
  // A drop-down list webform component.
  $options = array();
  $component = array ();
  $is_list = 1;
  $has_multiple = 0;
  $mandatory = 0;
  $webform_items = "Not Available";
  $webform_value = "";
  $webform_type = 'select';
  
  // If the tool_input contains a value of test_param,
  // For now, set the tool input equal to that
  if(array_key_exists("test_param", $tool_input)){
    $tool_input = $tool_input['test_param'];
  }
  
  
  // Set the default value accordingly.
  if(array_key_exists('value', $tool_input)){
    $webform_value = $tool_input['value'];
  }
  
  // Find the options
  if (array_key_exists ( 'options', $tool_input )) {
    $options = $tool_input['options'];
  }
  
  // Add options to the webform items list. This loop is tricky
  // But its essentially grabbing the first non-array item in each option.
  $x = 0;
  foreach ( $options as $option ) {
    if (count ( $option ) == 0) {
      continue;
    }
      
      // Iterate through the subArrays until we find our first string value.
    while ( is_array ( $option ) ) {
      
      // if the 'name' element exists, use this as our string value
      if (array_key_exists ( 'name', $option )) {
        $option = $option['name'];
      } else {
        
        // Let the option equal the first element in the option array
        foreach ( $option as &$inner_option ) {
          $option = $inner_option;
          break;
        }
      }
    }
    
    // Add the option to the webform_items in the form:
    // 'key1|value1 \n key2|value2'
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
      $webform_items = "t|True\nf|False";
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
    if(!array_key_exists('title', $tool_input)){
      $input_name = NULL;
    }else {
      $input_name = $tool_input['title'];
    }
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
    'cid' => $webform_cid,
    'name' =>"Step " . $index .": " . $input_name,
    'form_key' => "Step " . $index .": " . $input_name,
    'type' => $webform_type,
    'value' => $webform_value,
    'weight' => $webform_weight,  
    'mandatory' => $mandatory,
    'pid' => 0,
    'extra' => $extra,
    'page_num'=> $webform_page,
  );
  $webform_cid++;
  $webform_weight++;
  return $component;
}


/**
 * A funciton for building the workflow's input form
 */
function tripal_galaxy_admin_submitted_list() {
  tripal_galaxy_admin_submitted_list_base ( 0 );
}

/**
 * A bogus function that serves no real purpose other than to work as an
 *  example. 
 */
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