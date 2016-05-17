<?php
/**
 * @file
 * This file contains the functions used for administration of the module
 *
 */
/**
 * Admin home page for Tripal Galaxy 
 * 
 * @return 
 *   Formatted HTML table containing informaiton on all Galaxy Instances
 * 
 */
function tripal_galaxy_admin_home() {

  $sql = 'SELECT * FROM tripal_galaxy';  
  $results = db_query($sql);
  $results = $results->fetchAll();
  if($results== NULL || count($results) <= 0  ){
    return "No Galaxy Instances to display!";
  }

   $headers = array('Galaxy Server', 'Description', 'Options');
  
   $rows = array();
   // Populate the rows of the table
   foreach ($results as $result){
     $rows[] = array(
     $result->servername,
       $result->description,
        l( 'edit','/admin/tripal/extension/galaxy/edit/' . $result->galaxy_id),    
    );
   }
   
  $table = array(
      'header' => $headers,
      'rows' => $rows,
      'attributes' => array(
        'id' => 'tripal_galaxy-table-properties',
      'class' => 'tripal-data-table'
      ),
       'sticky' => FALSE,
        'caption' => '',
        'colgroups' => array(),
        'empty' => '',
    );
  
  return theme_table($table);
}

/**
 * Implement's Drupal's hook_form
 * 
 * @param $form
 * @param $form_state
 * @param $galaxy_id
 * @return $form
 */
function tripal_galaxy_admin_configuration_form($form, $form_state, $galaxy_id = NULL) {
  $default_server_name = "";
  $default_user_name = "";
  $default_description ="";
  $default_api_key="";
  $default_url ="";
  // If a galaxy id is set, feel free to declare all the default values with the appropriate
  // Galaxy instance
  if($galaxy_id !== NULL){
   $sql = "SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id";
     $galaxy_instance = db_query($sql, array(':galaxy_id' => $galaxy_id));
     $galaxy_instance = $galaxy_instance->fetchObject();
    
     if($galaxy_instance != NULL){
       $default_api_key = $galaxy_instance->api_key;
       $default_description = $galaxy_instance->description;
       $default_server_name = $galaxy_instance->servername;
       $default_user_name = $galaxy_instance->username;
       $default_url = $galaxy_instance->url;
     }
  }
  
  $form['galaxy_id'] = array(
    '#type' => 'value',
    '#value' => $galaxy_id,
  );
  
  // Server Name
  $form['servername'] = array(
    '#title' => t('Galaxy Server Name'),
    '#type' => 'textfield',
    '#size' => 50,
    '#default_value'=> $default_server_name,
    '#description'=>'Please provide the name of the remote Galaxy Server',
    '#required' => TRUE,
  );
  
  // Description
  $form['description'] = array(
      '#title' => t('Description'),
      '#type' => 'textarea',
      '#default_value'=> $default_description,
      '#description'=> 'Provide any additional details about this server.',
      '#required' => FALSE,
  );
  
  // URL
  $form['url'] = array(
      '#title' => t('URL'),
      '#type' => 'textfield',
      '#size' => 50,
      '#default_value'=> $default_url,
      '#description'=> 'The URL for the remote Galaxy server.',
      '#required' => TRUE,
  );
  
  // User Name
  $form['username'] = array(
      '#title' => t('User Name'),
      '#type' => 'textfield',
      '#size' => 50,
      '#default_value'=> $default_user_name,
      '#description'=> 'The user name for the Galaxy server.  
        This username is used to launch all jobs by default. 
        If this field is left blank then it is expected that the user
        has an account on the Galaxy server and will provide their username 
        when executing workflows.',
      '#required' => FALSE,
  );
  
  // API Key
  $form['api_key'] = array(
      '#title' => t('API Key'),
      '#type' => 'textfield',
      '#size' => 50,
      '#default_value'=> $default_api_key,
      '#description'=> "The API key for the user name specified above. 
        If this field is left blank then it is expected that the user 
        will provide their own API key when submitting a job.",
      '#required' => FALSE,
  );

  // Default submit
  $form['submit'] = array(
      '#type'=> 'submit',
      '#name' => 'submit',
      '#value' => t('Submit'),
  );
  
  // Delete
  $form['delete'] = array(
      '#type'=> 'submit',
      '#name' => 'delete',
      '#value' => t('Delete'),
  );

  
  return $form;
}

/**
 * Custom Validate for hook_form
 *     
 * @param $form
 * @param $form_state
 * @return boolean
 *   Return true if the form is valid for deletion, false otherwise
 */
function tripal_galaxy_admin_configuration_form_delete_validate($form, &$form_state){
  if ($form_state['clicked_button']['#name'] == 'delete') {
    // Check if a servername already exists, and that we're not editing a pre-existing
    // Instance
    if($form['tripal_galaxy_editing'] == FALSE){
      $sql = "SELECT * FROM {tripal_galaxy} WHERE servername=" . $form['tripal_galaxy_server_name']['#value'];
    
      $results = db_result(db_query($sql));
    
      if( $results != FALSE && $results != NULL){
        form_set_error('servername', t('A galaxy instance with that servername already exists'));
        return FALSE;
      }
    }  
  }
  else {
    // Check to make sure servername exists
    $sql = "SELECT * FROM {tripal_galaxy} WHERE servername=" . $form['tripal_galaxy_server_name']['#value'];
    $results = db_result(db_query($sql));
    if($results == NULL || $results == FALSE ){
      form_set_error('servername', t('A galaxy instance with that servername does not exist'));
  
    }
  }

}

/**
 * Implements Drupal's hook_form_submit
 *   Handles submitting a new galaxy instance to the Galaxy table 
 * 
 * @param $form
 * @param $form_state
 */
function tripal_galaxy_admin_configuration_form_submit($form, &$form_state) {
    $data = array (
     'servername' => $form_state['values']['servername'],    
     'description' => $form_state['values']['description'],
     'url' => $form_state['values']['url'],
     'username' => $form_state['values']['username'],
     'api_key' => $form_state['values']['api_key'],
    );
   
   if ($form_state['clicked_button']['#name'] == 'delete') {
      $galaxy_instance = db_query("DELETE FROM {tripal_galaxy} WHERE servername = %d", $form['tripal_galaxy_server_name']['#value']);
     
      // Set error if appripriate
      if($galaxy_instance === FALSE || $galaxy_instance == NULL) {
        drupal_set_message(t('Error deletingrecord'), 'error');
      } 
      else { // Redirect
        drupal_set_message(t('Galaxy instance has been deleted'));
        drupal_goto("/admin/tripal/extension/galaxy");
      }
   }
   else {
      // If there is a galaxy ID this will be an edit
      if ($form_state['values']['galaxy_id']) {
        $data['galaxy_id'] = $form_state['values']['galaxy_id'];
        $galaxy_instance = drupal_write_record('tripal_galaxy', $data, 'galaxy_id');
        
      }
      // Otherwise it's an edit
      else {
        $galaxy_instance = drupal_write_record('tripal_galaxy', $data);
      }
     // Insert the new Galaxy Instance into Drupal
   if(!$galaxy_instance){
    drupal_set_message(t('problem inserting galaxy instance'), 'error');
    drupal_goto("/admin/tripal/extension/galaxy");
    return;
   }
   drupal_set_message(t('Galaxy instance haghstrs been recorded' . var_dump($galaxy_instance)));
   drupal_goto("/admin/tripal/extension/galaxy");
   }   
  
}



/** 
 * Generates the form that lists the available workflows in the 
 *   default Galaxy instances and allows the site admin to select which ones 
 *   should be published on the site for end-users to use.
 * 
 * @param $form
 * @param $form_state
 */
function tripal_galaxy_admin_wf_available_form($form, &$form_state){

  // TODO: once the PHP bindings are up and running, replace this test workflow with information from 
  //  the actual galaxy server
  $workflows = array(
    'test_wf_1' => array(
       'description' => 'TODO: pull actual workflows from the server',
       'available' => TRUE,
       'workflow_name' => "Not actually pulled from any server1" 
    ),
      
    'test_wf_2' => array(
      'description' => 'TODO: pull actual workflows from the server',
    'available' => FALSE,
    'workflow_name' => "Not actually pulled from any server2"
     ),
  );
  
  $galaxy_id = '';
  if (array_key_exists('values', $form_state)) {
  	$galaxy_id = $form_state['values']['galaxy_id'];
  }
     
  $results = db_select('tripal_galaxy', 'tg')
    ->fields('tg', array('galaxy_id', 'servername', 'label'))
    ->execute();
  $galaxies[0] = 'Select';
  while ($galaxy = $results->fetchObject()) {
  	$galaxies[$galaxy->galaxy_id] = $galaxy->servername;
  }
  $i = 0;
  $form['galaxy_id'] = array(
  	'#type' => 'select',
    '#options' => $galaxies,
    '#title' => t('Galaxy Servers'),
    '#description' => t('Please select a galaxy instance to view it\'s workflows'),
  	'#ajax' => array(
      'callback' => 'tripal_galaxy_admin_wf_available_form_ajax_callback',
      'wrapper' => 'tripal-galaxy-admin-wf-available-form-rows',
      'method' => 'replace',
      'effect' => 'fade',
    ),
  	'#default_value' => $galaxy_id,
  );

  $form['#prefix'] = '<div id="tripal-galaxy-admin-wf-available-form-rows">';
  $form['#suffix'] = '</div>';
  
  if ($galaxy_id) {

	  foreach($workflows as &$workflow){
	      //dpm($workflow);  
	      
	      // First let's check to see if this workflow exists in our database  
	      $workflow_exists = 1;
	      $default_label = "";
	      $default_available = 0;
	      $sql = "SELECT * FROM {tripal_galaxy_wf} WHERE workflow_name = :workflow_name"; 
	      $workflow_instance = db_query($sql, array(':workflow_name' => $workflow['workflow_name']));
	      $workflow_instance = $workflow_instance->fetchObject();
	      if($workflow_instance == NULL || $workflow_instance == FALSE){
	        $workflow_exists = 0;
	      }
	      else {
	        $default_label = $workflow_instance->label;
	      }
	    
	    // Workflow name
	    $form['rows']['row' . $i]['workflow_name'. $i] = array(
	    '#type' => 'item',
	    '#markup' => $workflow['workflow_name'],
	   );

	    // Description
	    $form['rows']['row' . $i]['description'. $i] = array(
	      '#type' => 'item',
	      '#markup' => $workflow['description'],
	    );
	  
	    // Is available
	    $form['rows']['row' . $i]['available'. $i] = array(
	      //'#title' => t('checkbox'),
	      '#type' => 'checkbox',
	      '#default_value' => $workflow_exists,
	    );
	  
	    // Workflow_label
	    $form['rows']['row' . $i]['label'. $i] = array(
	      '#type' => 'textfield',
	      '#size' => 25,
	      '#default_value'=> $default_label,
	    );
	  
	    $i = $i + 1;
	  }	  
	  $form['rows']['#theme'] = 'tripal_galaxy_admin_wf_available_form_rows';
	  
	  // Default submit
	  $form['submit'] = array(
		  '#type'=> 'submit',
		  '#name' => 'submit',
		  '#value' => t('Save'),
	  );
  }

  
  
  //dpm($form);
  return $form;
}
/**
 * 
 */
function tripal_galaxy_admin_wf_available_form_ajax_callback(&$form, $form_state){
  return $form;
}
/**
 * TODO: Where do I place this?
 * @param $form
 * @param $form_state
 */
function theme_tripal_galaxy_admin_wf_available_form_rows($variables){
  $form = $variables['form'];
  $headers = array(
      //'Sequence<br>Owner',
      'Workflow Name',
      'Description',
      'Available?',
      'On site Label',
  );
  
  $rows = array();
  $i =0;
  foreach(element_children($form) as $element) {
    $rows[] = array(
      drupal_render($form[$element]["workflow_name". $i]),
      drupal_render($form[$element]["description". $i]),
      drupal_render($form[$element]["available". $i]),
      drupal_render($form[$element]["label". $i]),
    );
    $i = $i + 1;
  }

  // Subset sequences.
  return theme_table(array(
    'header'     => $headers,
    'rows'       => $rows,
    'attributes' => array(),
    'sticky'     => TRUE,
    'caption'    => '',
    'colgroups'  => array(),
    'empty'      => 'No Workflows Available',
  ));
}

//TODO: Validates the tripal_galaxy_admin_publish_form()
function tripal_galaxy_admin_wf_available_form_valiate(){

}

/**
	Implements Drupal's Hook_form_submit
		possible conditions: 
		  - Workflow already exists on site and needs to be updated
		  - Workflow does not exist on site and needs to be inserted
 */
function tripal_galaxy_admin_wf_available_form_submit($form, &$form_state){
	dpm($form);
	dpm($form_state);
	
	$i =0;
	while($i < count($form_state['values'])/3 ){
	  // If index i is out of range, break  
	  if(! array_key_exists('available' . $i, $form_state['values'])){
	    break;
	  }
	  $workflow_exists = FALSE;
	  // Select the workflow at a given location, if available.
	  $sql = "SELECT * FROM {tripal_galaxy_wf} WHERE workflow_name = :workflow_name";
	  $workflow_instance = db_query($sql, array(':workflow_name' => $form_state['complete form']['rows']['row' . $i]['workflow_name' . $i]['#markup']));
	  $workflow_instance = $workflow_instance->fetchObject();
	  
	  if($workflow_instance != NULL && $workflow_instance != FALSE){
	  	$workflow_exists = TRUE;
	  }
	  // If this workflow is selected to be added to the site...
	  if($form_state['values']['available'. $i]== 1){
	  	
	  	// If this workflow already exists on the database...
	  	if($workflow_exists){
	  		// If a field is different from our databse record, re-write the record 
	  		if( $workflow_instance->label != $form_state['values']['label'. $i] ||
	  			$workflow_instance->workflow_name != $form_state['complete form']['rows']['row' . $i]['workflow_name' . $i]['#markup']
	  		 ){
	  			$data = array(
	  			  'galaxy_wf_id'=> $workflow_instance->galaxy_wf_id,
	  			  'workflow_name'=> $form_state['complete form']['rows']['row' . $i]['workflow_name' . $i]['#markup'],
	  			  'label' => $form_state['values']['label'. $i],
	 			  'description' => $form_state['complete form']['rows']['row' . $i]['description' . $i]['#markup'],
	  			  'available' => $form_state['values']['available'. $i],	
	  			);
	  			$workflow = drupal_write_record('tripal_galaxy_wf', $data, 'galaxy_wf_id');
	  		}
	  	}else{ // If workflow doesn't exist
	  		$data = array(
	  		  'workflow_name'=> $form_state['complete form']['rows']['row' . $i]['workflow_name' . $i]['#markup'],
	  		  'label' => $form_state['values']['label'. $i],
	  		  'available' => $form_state['values']['available'. $i],
	  		  'description' => $form_state['complete form']['rows']['row' . $i]['description' . $i]['#markup'],
	  		);
	  		$workflow = drupal_write_record('tripal_galaxy_wf', $data);
	  	}
	  }else{ // If the workflow is unselected
	  	// If the workflow exists, delete the instance. 
	  	if($workflow_exists){
	  		$deleted = db_query("DELETE FROM {tripal_galaxy_wf} WHERE galaxy_wf_id = :wf_id", array('wf_id'=>$workflow_instance->galaxy_wf_id));
	  	}
	  }
	  
	  $i = $i+1;
	}
}

//TODO: Generates a table that provides a listing of the Galaxy workflow usage statistics  
// This is a form because it should provide filters (i.e to filter rows by user, etc.).
function tripal_galaxy_admin_usage_form(){
	
}

//TODO: Validates the tripal galaxy usage from.
function tripal_galaxy_admin_usage_form_validate(){
  
}

//TODO: Submits the admin usage form.
function tripal_galaxy_admin_usage_form_submit(){
  
}

//TODO: Generates a table that provides a listing of the Galaxy workflows that have been executed 
// and submitted by the user for the data to be incorporated into the site.  
// The table should provide links to download and view the results.
function tripal_galaxy_admin_submitted_list(){

}


function tripal_galaxy_admin_file_listing() {
  $output = '';

  // set the breadcrumb
  $breadcrumb = array();
  $breadcrumb[] = l('Home', '<front>');
  $breadcrumb[] = l('Administration', 'admin');
  $breadcrumb[] = l('Tripal', 'admin/tripal');
  $breadcrumb[] = l('Chado', 'admin/tripal/chado');
  $breadcrumb[] = l('galaxy', 'admin/tripal/chado/tripal_galaxy');
  drupal_set_breadcrumb($breadcrumb);


  // EXPLANATION:  Typically for all Tripal modules the home administrative
  // page for the module contains a search form to help the adminstrator
  // locate records.  The following galaxy code adds a default View to
  // the page
  /*
  // Add the view
  $view = views_embed_view('tripal_galaxy_admin_galaxy','default');
  if (isset($view)) {
    $output .= $view;
  }
  else {
    $output .= '<p>The Tripal galaxy Module uses primarily views to provide an '
      . 'administrative interface. Currently one or more views needed for this '
      . 'administrative interface are disabled. <strong>Click each of the following links to '
      . 'enable the pertinent views</strong>:</p>';
    $output .= '<ul>';
      $output .= '<li>'.l('galaxy Admin', 'admin/tripal/chado/tripal_galaxy/views/galaxy/enable').'</li>';
    $output .= '</ul>';
  }
  */
  $output = 'Typically a search view goes here';

  return $output;
}

/**
 * Administrative settings form
 *
 * @ingroup tripal_galaxy
 */
function tripal_galaxy_admin() {
  $form = array();

  $form['nothing'] = array(
    '#markup' => t('There are currently no settings to configure.')
  );

  // If your module is using the Chado Node: Title & Path API to allow custom titles
  // for your node type then you need to add the configuration form for this functionality.
  $details = array(
    'module' => 'tripal_galaxy',       // the name of the MODULE implementing the content type

    'unique_option' => '[tripal_galaxy.galaxy_id]'
  );
  // This call adds the configuration form to your current form
  // This sub-form handles it's own validation & submit
  chado_add_admin_form_set_title($form, $form_state, $details);

  return system_settings_form($form);
}



/**
 *
 * @ingroup tripal_galaxy
 */
function tripal_galaxy_admin_validate($form, &$form_state) {

}

