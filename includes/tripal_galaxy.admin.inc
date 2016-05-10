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

//TODO: Generates the form that lists the available workflows in the 
// default Galaxy instances and allows the site admin to select which ones 
// should be published on the site for end-users to use.
function tripal_galaxy_admin_publish_form(){

}

//TODO: Validates the tripal_galaxy_admin_publish_form()
function tripal_galaxy_admin_publish_form_valiate(){

}

//TODO: Is executed when the submit button is clicked in the tripal_galaxy_admin_publish_form()
function tripal_galaxy_admin_publish_form_submit(){

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
