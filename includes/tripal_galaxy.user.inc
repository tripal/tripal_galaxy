<?php 


function tripal_galaxy_admin_submitted_list(){
 
$node = new stdClass();
$node->type = 'webform';
node_object_prepare($node);
$node->title = 'Contact Us';
$node->language = 'en';
$node->body[LANGUAGE_NONE][0]['value']   = '';
$node->body[LANGUAGE_NONE][0]['format']  = 'full_html';
$node->uid = 1;
$node->promote = 0;
$node->comment = 0;

// Create the webform components.
$components = array(
  array(
  'name' => 'Workflow',
  'form_key' => 'selector',
  'type' => 'select',
  'mandatory' => 1,
  'weight' => 0,
  'pid' => 0,
  'extra' => array(
    'title_display' => 'inline',
    'private' => 0,
    'items' => "workflow1|workflow2|workflow3|Mr_workflow",
    'aslist' => 1,
  ),
),
array(
  'name' => 'Name',
  'form_key' => 'name',
  'type' => 'textfield',
  'mandatory' => 1,
  'weight' => 5,
  'pid' => 0,
  'extra' => array(
    'title_display' => 'inline',
    'private' => 0,
  ),
),

array(
  'name' => 'Input1',
  'form_key' => 'input',
  'type' => 'select',
  'mandatory' => 0,
  'weight' => 20,
  'pid' => 0,
  'extra' => array(
    'title_display' => 'inline',
    'private' => 0,
    'aslist' => 1,
    'options_source' => "workflow1|workflow2|workflow3|Mr_workflow",
  ),
),
array(
  'name' => 'Email address',
  'form_key' => 'email_address',
  'type' => 'email',
  'mandatory' => 1,
  'weight' => 25,
  'pid' => 0,
  'extra' => array(
    'title_display' => 'inline',
    'private' => 0,
  ),
),
array(
  'name' => 'Subject',
  'form_key' => 'subject',
  'type' => 'select',
  'mandatory' => 1,
  'weight' => 30,
  'pid' => 0,
  'extra' => array(
    'title_display' => 'inline',
    'private' => 0,
    'items' => "s1|Subject 1\nother|Other",
    'aslist' => 1,
  ),
),
  array(
    'name' => 'Mandatory Fields',
    'form_key' => 'mandatory_fields',
    'type' => 'markup',
    'mandatory' => 0,
    'weight' => 40,
    'pid' => 0,
    'value' => 'Fields with * are mandatory',
    'extra' => array(
      'title_display' => 'inline',
      'private' => 0,
      'format'=> 'full_html',
    ),
  ),
);

// Setup notification email.
$emails = array(
  array(
/*     'email' => '',
    'subject' => 'default',
    'from_name' => 'default',
    'from_address' => 'default',
    'template' => 'default',
    'excluded_components' => array(), */
  ),
);

// Attach the webform to the node.
$node->webform = array(
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
  'roles' => array(
    0 => '1', // Anonymous user can submit this webform.
  ),
  'emails' => $emails,
  'components' => $components,
);
  
  // Save the node.
  node_save ( $node );

  
    // Initialize the headers and rows of the table.
  $rows = array();
  $headers = array('',);


  while ($result = $results->fetchObject()) {
    $rows[] = array($result->servername, $result->description,
      l('edit', '/admin/tripal/extension/galaxy/edit/' . $result->galaxy_id));
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
    'empty' => 'No Galaxy instances have been added.'
  );
  return theme_table($table);
  
  
}


?>