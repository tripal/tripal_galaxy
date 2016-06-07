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
  
  // Build the web form based on the input steps
  
  $components = NULL;
  
  // Make sure we have valid input
  if ($workflow == NULL || ! is_array ( $workflow ) || ! array_key_exists ( 'steps', $workflow )) {
    return;
  }
  
  $i = count ( $workflow['steps'] );
  foreach ( array_reverse ( $workflow['steps'] ) as $step ) {
    
    dpm ( "The step: " );
    dpm ( $step );
    
    // If we are uploading something...
    $components[$i] = array (
      'name' => 'Step ' . ($i),
      'form_key' => 'the_step_' . ($i),
      'type' => 'markup',
      'weight' => 26,
      'value' => 'Step ' . ($i),
      'mandatory' => 0,
      'pid' => 0 
    );
    
    // If the tool wants a dataset to be uploaded...
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
      // Get the tool_id, Continue if it is null;
      $tool_id = $step['tool_id'];
      if ($tool_id == NULL) {
        $i ++;
        continue;
      }
      
      // Create a tool object
      dpm ( 'STARtING!!!!!!!!!!!!!!!!!!!!!!!!11111' );
      $tool_instance = new GalaxyTools ( $galaxy_instance );
      $tool = $tool_instance->build ( array (
        'tool_id' => $tool_id,
        'history_id' => $target_history['id'] 
      ) );
      dpm ( "This is the Tool:" );
      dpm ( $tool_id );
      dpm ( $tool );
      dpm ( $galaxy_instance->getErrorMessage () );
      if($tool == FALSE || $tool == NULL){
        drupal_set_message('Error creating form for this workflow', 'Error');
        return;
      }
      // Iterate over tool inputs
      foreach ( $tool["inputs"] as $input ) {
        
        dpm ( "This is the input: " );
        dpm ( $input );
        
        // Add the label to the components
        $components[] = array (
          'name' => 'input tool ' . ($i) . $input['label'],
          'form_key' => 'Input ' . ($i) . ": " . $input['label'],
          'type' => 'markup',
          'weight' => 26,
          'value' => 'Input ' . ($i) . ": " . $input['label'],
          'mandatory' => 0,
          'pid' => 0 
        );
        // Add components to match each of the input models
        $components[] = construct_component ( $input );
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
    ), // Anonymous user can submit this webform.
      // 'emails' => $emails,
    'components' => $components 
  );
  
  // Save the node.
  node_save ( $node );
}

/**
 * A function to construct a web form component based on 
 * A given tool input
 * 
 * @param $tool_input 
 *   The tool input to build the component for 
 *  
 * @return 
 *   An array suitable for the 'components' webform array 
 */
function construct_component($tool_input) {
  $options = array();
  if(array_key_exists('options', $tool_input)){
    $options = $tool_input['options'];
  }
  
  dpm("these are the options: ");
  dpm($options);
  
  // These are the cases
  switch ($tool_input) {
    case 'text' :
      break;
    case 'select' :
      break;
    

    case 'data_column':
      break;
    case 'genomebuild':
      break;
    
    // Usually refers to the data coming into the tool
    case 'data':
      break;
    case 'data_collection':
      break;
    case 'integer':
      break;
    case 'float':
      break;
    case 'boolean':
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
  }
}


/**
 * A funciton for building the workflow's input form
 */
function tripal_galaxy_admin_submitted_list() {
  tripal_galaxy_admin_submitted_list_base ( 0 );
}


?>