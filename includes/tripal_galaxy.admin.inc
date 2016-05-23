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
 *   Formatted HTML table containing information on all Galaxy Instances
 *
 */

require_once(dirname(__FILE__) . "/../galaxy_api/GalaxyLib-PHP/src/GalaxyInstance.inc");
require_once(dirname(__FILE__) . "/../galaxy_api/GalaxyLib-PHP/src/Workflows.inc");

function tripal_galaxy_admin_home() {


  // Initialize the headers and rows of the table.
  $rows = array();
  $headers = array('Galaxy Server', 'Description', 'Options');

  // Retrieve the list of galaxy servers that have been added and generate
  // the rows for the table.
  $results = db_select('tripal_galaxy', 'tg')
    ->fields('tg')
    ->execute();
  while($result = $results->fetchObject()) {
    $rows[] = array(
      $result->servername,
      $result->description,
      l('edit','/admin/tripal/extension/galaxy/edit/' . $result->galaxy_id),
    );
  }

  // Theme the table and return.
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
    'empty' => 'No Galaxy instances have been added.',
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

  // The galaxy id
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
	
	// Check to see if we are deleting an instance. 
	if ($form_state['clicked_button']['#name'] == 'delete') {
	  $sql = "SELECT * FROM {tripal_galaxy} WHERE servername=" . $form['tripal_galaxy_server_name']['#value'];
	  $results = db_result(db_query($sql));
	  if($results == NULL || $results == FALSE ){
	  	
	  	// If there is no entry with this servername return False. 
	    form_set_error('servername', t('A galaxy instance with that servername does not exist'));
	    return FALSE;
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
    
    // Initialize the data to be sent to the tripal_galaxy table
	$data = array (
     'servername' => $form_state['values']['servername'],
     'description' => $form_state['values']['description'],
     'url' => $form_state['values']['url'],
     'username' => $form_state['values']['username'],
     'api_key' => $form_state['values']['api_key'],
    );
   // If we are deleting the given instance, go through the appropriate motions...
   if ($form_state['clicked_button']['#name'] == 'delete') {
      $galaxy_instance = db_query("DELETE FROM {tripal_galaxy} WHERE servername = :servername",array(':servername' => $form_state['values']['servername']));

      // Set error if appripriate
      if($galaxy_instance === FALSE || $galaxy_instance == NULL) {
        drupal_set_message(t('Error deleting record'), 'error');
        return;
      }
      else { 
      	//If there is no error, redirect. 
        drupal_set_message(t('Galaxy instance has been deleted'));
        drupal_goto("/admin/tripal/extension/galaxy");
      }
   }
   // If we are not deleting, we must be submitting. 
   else {
      // If there is a galaxy ID this will be an edit
      if ($form_state['values']['galaxy_id']) {
        $data['galaxy_id'] = $form_state['values']['galaxy_id'];
        $galaxy_instance = drupal_write_record('tripal_galaxy', $data, 'galaxy_id');
      }
      // Otherwise it's a submit
      else {
        $galaxy_instance = drupal_write_record('tripal_galaxy', $data);
      }
      
   // Insert the new Galaxy Instance into Drupal, return where appropriate
   if(!$galaxy_instance){
    drupal_set_message(t('problem inserting galaxy instance'), 'error');
    drupal_goto("/admin/tripal/extension/galaxy");
    return;
   }
   
   // Default message.
   drupal_set_message(t('Galaxy instance has been recorded' . var_dump($galaxy_instance)));
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
 
  // Grab the galaxy id from the form state and Obtain the default galaxy server
  // from the database.
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
  
  // Select which galaxy server to choose the workflows from
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
  
  // If a galaxy instance has been selected, Ajax-complete the form
  if ($galaxy_id) {
  	
  	// Grab the appropriate galaxy server table entry
  	$galaxy_server = db_query("SELECT * FROM {tripal_galaxy} WHERE galaxy_id = :galaxy_id", array(':galaxy_id' => $galaxy_id));
	$galaxy_server = $galaxy_server->fetchObject();


	// Obtain the hostname and port from the url
    $matches = array();
    $regex = "((https|http):\/\/)?"; // SCHEME
    $regex .= "([a-z0-9-.]*)"; // Host or IP
    $regex .= "(:)?([0-9]*)?"; // Port	
    preg_match("/^$regex$/", $galaxy_server->url, $matches);
    $hostname = $matches[3];
    $port = $matches[5];
    
  	//TODO: HTTP vs HTTPS 
  	
    // Create a new galaxy instance using the obtained hostname and port
 	// Then Authenticate
  	$galaxy_instance = new GalaxyInstance($hostname, $port);
  	$galaxy_instance->setAPIKey($galaxy_server->api_key);
  	
  	// Obtain the workflows
  	$workflows = new Workflows($galaxy_instance);
  	$workflows = $workflows->index();

  	 // Loop through each workflow to add it to the form
	 foreach($workflows as &$workflow){

	   // First let's check to see if this workflow exists in our database,
	   // if it does, assign default values as appropriate
	   $workflow_exists = 1;
	   $default_label = "";
	   $default_available = 0;
	   $sql = "SELECT * FROM {tripal_galaxy_wf} 
	   		   WHERE workflow_id = :workflow_id 
	   		   AND galaxy_id = :galaxy_id";
	   $workflow_instance = db_query($sql, array(
	     ':workflow_id' => $workflow['id'],
	   	 ':galaxy_id' => $galaxy_id,	
	   ));
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
	   '#markup' => $workflow['name'],
	   );
	    
	   // Workflow_id
	   $form['rows']['row' . $i]['workflow_id'. $i] = array(
	     '#type' => 'item',
	     '#markup' => $workflow['id'],
	   );
	   
	   // Description
	   $form['rows']['row' . $i]['description'. $i] = array(
	    '#type' => 'item',
	    '#markup' => "Descriptions are not inherently part of workflows...",
	   );

	   // Is available
	   $form['rows']['row' . $i]['available'. $i] = array(
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
  
  return $form;
}

/**
 * Ajax Callback
 */
function tripal_galaxy_admin_wf_available_form_ajax_callback(&$form, $form_state){
  // Return the whole form.
  return $form;
}

/**
 * TODO: Where do I place this?
 * @param $form
 * @param $form_state
 */
function theme_tripal_galaxy_admin_wf_available_form_rows($variables){
  $form = $variables['form'];
  
  // The apparopriate headers for this form's table.
  $headers = array(
    'Workflow Name',
    'Description',
    'Available?',
    'On site Label',
  	'Id',
  );
  
  // Theme the rows of this form's table.
  $rows = array();
  $i =0;
  foreach(element_children($form) as $element) {
   $rows[] = array(
      drupal_render($form[$element]["workflow_name". $i]),
      drupal_render($form[$element]["description". $i]),
      drupal_render($form[$element]["available". $i]),
      drupal_render($form[$element]["label". $i]),
   	  drupal_render($form[$element]["workflow_id". $i]),
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

/**
 * Override's Drupal's Hook_form_validate
 *   Checks to make sure an entry with this label doesn't already exist  
 */
function tripal_galaxy_admin_wf_available_form_validate($form, &$form_state){
  $i =0;
	
  // Go through all of the row entries.
  while($i < count($form_state['values'])/4){
		 
    // If index i is out of range, break
	if(! array_key_exists('available' . $i, $form_state['values'])){
	  return;
	}
		
	// If the form entry is not selected to add to database,
	// Continue to the next form entry
	if($form_state['values']['available' . $i] == 0){
	  $i = $i+1;
	  continue;
	}
		
	// Select the workflow at the given index i, if available.
	$workflow_exists = FALSE;
	$sql = "SELECT * FROM {tripal_galaxy_wf} WHERE label = :label";
	$workflow_instance = db_query($sql, array(
      ':label' => $form_state['values']['label'. $i],
	));
	$workflow_instance = $workflow_instance->fetchObject();
	
	// If the selected workflow doesn't have the same id as the workflow
	// in the form, then a duplicate label is present. 
	if($workflow_instance != NULL && $workflow_instance != FALSE){
	  if($workflow_instance->workflow_id != $form_state['complete form']['rows']['row' . $i]['workflow_id' . $i]['#markup']){
	    form_set_error('label', t('A workflow with that label already exists'));
		return FALSE;
	  }
	}
		
    $i = $i+1;
  } 
	
	return;
}

/**
    Implements Drupal's Hook_form_submit
		possible conditions:
		  - Workflow already exists on site and needs to be updated.
		  - Workflow does not exist on site and needs to be inserted.
 */
function tripal_galaxy_admin_wf_available_form_submit($form, &$form_state){
	$i =0;
	dpm($form_state);
	// Go through all of the row entries. 
	while($i < count($form_state['values'])/4){
	  
	  // If index i is out of range, break
	  if(! array_key_exists('available' . $i, $form_state['values'])){
	    break;
	  }
	  
	  // Select the workflow at the given index i, if available. Note that label
	  // is a unique key.
	  $workflow_exists = FALSE;
	  $sql = "SELECT * FROM {tripal_galaxy_wf} WHERE workflow_id = :workflow_id AND galaxy_id = :galaxy_id";
	  $workflow_instance = db_query($sql, 
	  		array(
	  		  ':workflow_id' => $form_state['complete form']['rows']['row' . $i]['workflow_id' . $i]['#markup'],
	  		  ':galaxy_id' => $form_state['values']['galaxy_id'],
	  		));
	  $workflow_instance = $workflow_instance->fetchObject();
	  if($workflow_instance != NULL && $workflow_instance != FALSE){
	  	$workflow_exists = TRUE;
	  }
	  
	  dpm("Does the workflow exist?");
	  dpm($workflow_exists);
	  // If this workflow is selected to be added to the site...
	  if($form_state['values']['available'. $i] == 1){

	  	// If this workflow already exists on the database...
	  	if($workflow_exists){
	  		dpm($workflow_instance);
	  		
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
	  		
	  	// If workflow doesn't exist, add the new one to the database!
	  	}else{ 
	  		$data = array(
	  		  'workflow_name'=> $form_state['complete form']['rows']['row' . $i]['workflow_name' . $i]['#markup'],
	  		  'workflow_id' => $form_state['complete form']['rows']['row' . $i]['workflow_id' . $i]['#markup'],
	  		  'galaxy_id' => $form_state['values']['galaxy_id'],
	  		  'label' => $form_state['values']['label'. $i],
	  		  'available' => $form_state['values']['available'. $i],
	  		  'description' => $form_state['complete form']['rows']['row' . $i]['description' . $i]['#markup'],
	  		);
	  		$workflow = drupal_write_record('tripal_galaxy_wf', $data);
	  	}
	  	
	  // If the workflow is unselected
	  }else{
	  	
	  	// If the workflow exists, delete the instance.
	  	if($workflow_exists){
	  		$deleted = db_query("DELETE FROM {tripal_galaxy_wf} WHERE galaxy_wf_id = :wf_id", array(':wf_id'=>$workflow_instance->galaxy_wf_id));
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

