<?php
/**
 * @file
 * This file contains the functions used for administration of the module
 *
 */
// TODO: A function that returns a themeable array for the landing page for the Galaxy administrative pages.
function tripal_galaxy_admin_home() {
   $rows = array(
   	  array(
   	   'data' => array(
   	     "Galaxy 1", array('data'=> "The galaxy instance"),
   		   'class'=>array('funky'),
   		  )
   		)
   	);
	
   $header = "Find your table here!";
   $variables = array();
   $variables['rows'] = $rows;
   $variables['header'] = $header;
   $table = theme_table($variables);
	
	
   $page = array();
   
  '#show_messages' => TRUE,
  '#theme' => 'page',
  '#type' => 'page',
  'content' => array(
    'system_main' => array(...),
	  'another_block' => array(...),
	    '#sorted' => TRUE,
    ),
	'sidebar_first' => array(
			...
	),
	'footer' => array(
			...
	),
	...
	);
  
  
  
}

// TODO: Generates the form for configuring one or more default Galaxy instances.
function tripal_galaxy_admin_configuration_form() {
	
}

// TODO: Validates the tripal_galaxy_admin_configuration_form
function tripal_galaxy_admin_configuration_form_validate(){

}

//TODO: Is executed when the submit button is clicked in the tripal_galaxy_admin_configuration_form.
function tripal_galaxy_admin_configuration_form_submit() {

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
