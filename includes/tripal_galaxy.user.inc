<?php

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
  $anon_galaxy_instance = new GalaxyInstance ( $hostname, $port );
  
  $galaxy_instance = new GalaxyInstance ( $hostname, $port );
  $galaxy_instance->setAPIKey ( $galaxy_server->api_key );
  
  // Set up the workflow instance
  $workflow_instance = new Workflows ( $galaxy_instance );
  $workflow = $workflow_instance->show ( $workflow_id );
  
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
  $i = 0;
  $components = NULL;
  foreach ( array_reverse($workflow['steps']) as $step ) {
    
    // If we are uploading something...
    $components[] = array (
      'name' => 'Step ' . ($i+1),
      'form_key' => 'the_step_' . ($i+1),
      'type' => 'markup',
      'weight' => 26,
      'value' => 'Step ' . $i,
      'mandatory' => 0,
      'pid' => 0 
    )
    ;
    
    // If the tool wants a dataset to be uploaded...
    if ($step['tool_id'] == NULL && $step['type'] == "data_input") {
      $compnents[] = array (
        'name' => 'Upload dataset',
        'form_key' => 'dataset_' . $i,
        'type' => 'file',
        'weight' => 26,
        'mandatory' => 1,
        'pid' => 0 
      );
      $i ++;
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
      $tool_instance = new Tools ( $anon_galaxy_instance );
      $tool = $tool_instance->build ( array (
        'tool_id' => $tool_id 
      ) );
      dpm ( "This is the Tool:" );
      dpm ( $tool_id );
      dpm ( $tool );
      dpm ( $tool_instance->getErrorMessage () );
    }
    
    $i ++;
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
 * A funciton for building the workflow's input form
 */
function tripal_galaxy_admin_submitted_list() {
  tripal_galaxy_admin_submitted_list_base ( 0 );
}


?>