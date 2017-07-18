<?php

function tripal_galaxy_admin_usage_page() {

  // set the breadcrumb
  $breadcrumb = array();
  $breadcrumb[] = l('Home', '<front>');
  $breadcrumb[] = l('Administration', 'admin');
  $breadcrumb[] = l('Tripal', 'admin/tripal');
  $breadcrumb[] = l('Extensions', 'admin/tripal/extension');
  $breadcrumb[] = l('Galaxy', 'admin/tripal/extension/galaxy');
  drupal_set_breadcrumb($breadcrumb);

  tripal_add_d3js();
  drupal_add_js(drupal_get_path ('module', 'tripal_galaxy') . '/theme/js/tripal_galaxy.dashboard.js');
  drupal_add_css(drupal_get_path ('module', 'tripal_galaxy') . '/theme/css/tripal_galaxy.dashboard.css');

  // Get some basic stats

  $headers = array();
  $rows = array();

  // The number of workflow submissions
  $sql = "
    SELECT count(*) as count
    FROM {tripal_galaxy_workflow_submission} TGWS
  ";
  $result = db_query($sql);
  $rows[] = array(
    array(
      'data' => 'Total submisssions',
      'header' => TRUE,
      'width' => '50%'
    ),
    $result->fetchField(),
  );

  // The total number of hours executd.
  $sql = "
    SELECT count(name) FROM (
    SELECT DISTINCT U.name
    FROM {tripal_galaxy_workflow_submission} TGWS
      INNER JOIN {webform_submissions} WS ON TGWS.sid = WS.sid
      INNER JOIN {users} U on U.uid = WS.uid ) as users
  ";
  $result = db_query($sql);
  $rows[] = array(
    array(
      'data' => 'Users Submitting Workflows',
      'header' => TRUE,
    ),
    $result->fetchField(),
  );

  // The total number of hours executd.
  $sql = "
    SELECT sum(duration) as total_seconds
    FROM (
      SELECT (end_time - start_time) as duration
      FROM {tripal_galaxy_workflow_submission} TGWS
    ) as worflows
  ";
  $result = db_query($sql);
  $rows[] = array(
    array(
      'data' => 'Total execution time',
      'header' => TRUE,
    ),
    $result->fetchField(),
  );

  $stats = theme_table(array(
    'header' => $headers,
    'rows' => $rows,
    'attributes' => array(
      'id' => 'tripal-galaxy-usage-stats-table',
    ),
    'sticky' => FALSE,
    'caption' => '',
    'colgroups' => array(),
    'empty' => '',
  ));


  //  Get the top 10 users.
  $sql = "
    SELECT U.name, count(*) as count
    FROM {tripal_galaxy_workflow_submission} TGWS
      INNER JOIN {webform_submissions} WS ON TGWS.sid = WS.sid
      INNER JOIN {users} U on U.uid = WS.uid
    GROUP BY U.name
    LIMIT 10 OFFSET 0
  ";
  $results = db_query($sql);
  $top10users = array();
  while ($result = $results->fetchObject()) {
    $top10users[] = array(
      'name' => $result->name,
      'count' => $result->count,
    );
  }

  // Get the top 10 workflows.
  $sql = "
    SELECT N.title as name, count(*) as count
    FROM {tripal_galaxy_workflow_submission} TGWS
      INNER JOIN {tripal_galaxy_workflow} TGW ON TGWS.galaxy_workflow_id = TGW.galaxy_workflow_id
      INNER JOIN {node} N on N.nid = TGW.nid
    GROUP BY N.title
    LIMIT 10 OFFSET 0
  ";
  $results = db_query($sql);
  $top10workflows = array();
  while ($result = $results->fetchObject()) {
    $top10workflows[] = array(
      'name' => $result->name,
      'count' => $result->count,
    );
  }

  drupal_add_js("var top10users = " . json_encode($top10users) . ";", array('type' => 'inline'));
  drupal_add_js("var top10workflows = " . json_encode($top10workflows) . ";", array('type' => 'inline'));

  $content = "
    <div id=\"tripal-galaxy-usage-stats\" class=\"tripal-galaxy-usage-pane\">
      <h2>Usage Stats</h2>
      $stats
    </div>
    <div id=\"tripal-galaxy-usage-users\" class=\"tripal-galaxy-usage-pane\">
      <h2>Top 10 Submitters</h2>
      <p>The following chart shows the top 10 users that have submitted workflows.</p>
      <div id=\"tripal-galaxy-usage-users-chart\"></div>
    </div>
    <div id=\"tripal-galaxy-usage-workflows\" class=\"tripal-galaxy-usage-pane\">
      <h2>Top 10 Workflows Submitted</h2>
      <p>The following chart shows the top 10 workflows that have been submitted.</p>
      <div id=\"tripal-galaxy-usage-workflows-chart\"></div>
    </div>
  ";
  return array(
    '#type' => 'markup',
    '#markup' => $content,
  );
}

/**
* Provides the means of setting quotas and seeing server consumption.
*
* @return A table of the current users with custom quotas, fieldsets for
*         adding new users to the custom quotas, and fieldsets for setting
*         site wide quota and expiration date.
*/
function tripal_galaxy_admin_quota_usage($form, &$form_state) {
  // Provide overall server consumption (and space remaining)

  // Query file usage table for the fids that the galaxy module uses
  $sql = "
    SELECT DISTINCT fid
    FROM {file_usage}
    WHERE module = 'tripal_galaxy'";
    $fids = db_query($sql);
    // TODO: Ask stephen about a more efficient method, I'm sure there's one
    // but I'm just getting this done for now
    $total_size = 0;

    // Query the file managed table to get the total file consumption of
    // the tripal_galaxy module's users
    while($fid = $fids->fetchObject()) {
      // print_r($fid->fid . "\n");
      $sql = "
      SELECT filesize
      FROM {file_managed}
      WHERE fid = :fid";
      $total_size += db_query($sql, array (':fid' => $fid->fid))->fetchObject()->filesize;
    }
    // print_r($total_size . "\n");

    // XXX: Ask Stephen if there's something more graceful than this
    // NOTE: this file size is stored in bytes so
    $kb = 1024;
    $mb = pow(1024, 2);
    $gb = pow(1024, 3);
    $tb = pow(1024, 4);
    // Create a switch statment here to present the consumption in either MB, GB,
    // or TB, truncate to the 100th's place.
    $human_readable_size = "";
    switch($total_size){
      case (($total_size / $tb) >= 1):
      $human_readable_size = (string)floor(($total_size / $tb) * 100) / 100 . " TB";
      break;
      case (($total_size / $gb) >= 1):
      $human_readable_size = (string)floor(($total_size / $gb) * 100) / 100 . " GB";
      break;
      case (($total_size / $mb) >= 1):
      $human_readable_size = (string)floor(($total_size / $mb) * 100) / 100 . " MB";
      break;
      case (($total_size / $kb) >= 1):
      $human_readable_size = (string)floor(($total_size / $kb) * 100) / 100 . " KB";
      break;
      // Measure in bytes
      default:
      $human_readable_size = (string)floor(($total_size) * 100) / 100 . " Bytes";
      break;

    }
    $form['total_size'] = array (
      '#type' => 'markup',
      '#markup' => "<h3>Total Current Usage</h3> <p>$human_readable_size</p><p>The total amount of space consumed by user file uploads.</p>",
      );

  // Provide 2 field sets and a button above the table, these two field sets
  // provide site wide defaults to the quota size and the expiration limit
  // the button is to update the proper drupal variables
  $form['default_quota'] = array (
    '#type' => 'textfield',
    '#title' => 'Default System-Wide User Quota',
    '#description' => 'Set the maximum amount of space on the server that a single user can consume.
                       This default applies to all users who upload files using the TripalUploader tool.
                       Use the form below to set individual quotas on a per use basis.<br></br>
                       Suffixes include MB, GB, TB. Leaving the suffix blank means the suffix will be MB.',
    '#default_value' => 200,
  );

  $form['default_expiration_date'] = array (
    '#type' => 'textfield',
    '#title' => 'Default System-Wide Expiration Date',
    '#description' => 'The number of days that a user uploaded file can remain on the server before it is automatically removed',
    '#default_value' => 60,
  );

  $form['update_defaults'] = array (
    '#type' => 'submit',
    '#value' => t('Update Defaults'),

  );

  // Populate the table from the custom quota db table (users, quota, exp date).
  $header = array(
    'uid' => t('UID'),
    'user' => t('Users'),
    'custom_quota' => t('Custom Quota'),
    'exp_date' => t('Expiration Date'),
    'actions'    => t('Actions'),
  );

  // API call to the gather the users that have a custom quota
  $rows = array();

  $rows = tripal_galaxy_gather_user_custom_quotas('all');

  // Add the Actions links
  foreach($rows as $key => $entry){
    $rows[$key]['actions'] = l('Remove', '/admin/tripal/files/remove/' . $entry['uid']) . '  ' . l('Edit', '/admin/tripal/files/user/edit_user/' . $entry['uid']);
  }

  // Within the table (bottom row) provide a menu local action to add a new
  // user to the custom table

  // NOTE: If the user already exists as the admin is attempting to add an
  // existing user, create a validate for this function to ask if they want to
  // overwrite the existing quota/exp_date for that user.
  $rows[] = array(
    'uid' => '',
    'user' => l('Add User', '/admin/tripal/files/add_user'),
    'custom_quota' => '',
    'exp_data' => '',
    'actions' => ''
  );

  $form['table_title'] = array (
    '#type' => 'markup',
    '#markup' => '<br></br><h3>Per User Quotas</h3>',
  );
   $form['table'] = array (
     '#theme' => 'table',
     '#header' => $header,
     '#rows' => $rows,
   );

   return $form;
}

/**
 * Validate the form's values: proper numbers and/or MB, GB, TB for quota field.
**/
function tripal_galaxy_admin_quota_usage_validate($form, &$form_state) {
  // Check to make sure the admin has values in the default quota and
  // expiration box and that the quota has either MB, GB, TB, or no other
  // letters (defaults to MB)

}

/**
 * Write to the two drupal variables the site wide defualt quota and exp date.
**/
function tripal_galaxy_admin_quota_usage_submit($form, &$form_state) {
  // Grab the quota value and exp_date to write to the variables
  drupal_set_message(t('Y hello there.'));
}

/**
 * Form for adding a user to the quota
 *
 * @return Associative array of users' non-defualt quota and exp_date
**/
function tripal_galaxy_add_user_custom_quota($form, &$form_state) {

  // Textfield (ajax call based off of existing users) for users on the site
  $form['find_user'] = array (
    '#type' => 'textfield',
    '#title' => 'User',
    '#autocomplete_path' => 'admin/tripal/files/user/autocomplete',
  );

  // Custom quota textfield (prepopulated with defualt value)
  $form['custom_quota'] = array (
    '#type' => 'textfield',
    '#title' => 'Custom User Quota',
    '#description' => 'Set the maximum amount of space on the server that a  user can consume.
    <br></br>Suffixes include MB, GB, TB. Leaving the suffix blank means the suffix will be MB.',
    '#default_value' => 200,
  );

  // Custom exp date textfield (prepopulated with defualt value)
  $form['custom_expiration_date'] = array (
    '#type' => 'textfield',
    '#title' => 'Custom User Expiration Date',
    '#description' => 'The number of days that a user uploaded file can remain on the server before it is automatically removed.',
    '#default_value' => 60,
  );

  // Submit button
  $form['add_user'] = array (
    '#type' => 'submit',
    '#value' => t('Add User'),
  );

  return $form;

}

/**
 * Autocomplete function for listing existing users on the site.
 *
 * @return json array of users that match the query in the textfield
 **/
function _users_autocomplete($string) {
  $matches = array();
  $result = db_select('users', 'u')
  ->fields('u', array('name'))
  ->condition('name', '%' . db_like($string) . '%', 'LIKE')
  ->execute();

  foreach ($result as $row) {
    $matches[$row->name] = check_plain($row->name);
  }
  drupal_json_output($matches);
}

/**
 * Ensure the values in the textfields are valid and that the user is new.
 *
**/
function tripal_galaxy_add_user_custom_quota_validate($form, &$form_state) {
  // Check that the user being added is unique to the table else run an extra
  // form

  // Get the UID of the given user
  $uid = db_query("SELECT uid
                   FROM {users}
                   WHERE name = :name",
                   array(':name' => $form['find_user']['#value']))
                   ->fetchObject()->uid;

  // Run the query on the tripal_galaxy_custom_quota
  $result = db_query("SELECT uid
                      FROM {tripal_galaxy_custom_quota}
                      WHERE uid = :uid",
                      array(':uid' => $uid));

  // If there is an existing entry in the table with the given uid launch the
  // confirmation page

  // How to send the contents of this form to the overwrite function?
  // HACK: using the url to send the values, this is crappy but it works
  if($entry = $result->fetchObject()) {
    drupal_goto('/admin/tripal/files/user/overwrite_user/confirm/' . $uid . '/' . $form['custom_quota']['#value'] . '/' . $form['custom_expiration_date']['#value']);
  }

  // Check that the values of quota and expiration date are numberical and with
  // digital storage nomenclature

  // Validate the qutoa
  $matches = array();
  preg_match("/[^\-\d](\d+) ?(MB|GB|TB)?$/", $form['custom_quota']['#value'], $matches);
  // $matches[0] will be the numeric value
  // $matches[1] will be the digital storage size: MB, GB, TB, or blank
  //   (defaults to MB)
  // No numeric found
  if(!array_key_exists(0, $matches)) {
    form_set_error('custom_quota', t('Please enter a positive <b>integer</b> value as the quota'));
  }
  else {
    $form['custom_quota']['#value'] = $matches[0];
  }
  // if the 2nd element is occupied by a valid suffix we need to check to see
  // what we have to multiply the value by (1024 for GB 1024^2 for TB because
  // we assume that the initial number is already in MB)
  if(array_key_exists(1, $matches)) {
    switch ($matches[1]) {

      case 'GB':
        // Multiply $matches[1] by 1024 and overwrite
        // $form['custom_quota']['#value']
        $form['custom_quota']['#value'] = (int)$form['custom_quota']['#value'] * 1024;
        break;

      case 'TB':
      // Multiply $matches[1] by 1024 and overwrite
      // $form['custom_quota']['#value']
      $form['custom_quota']['#value'] = (int)$form['custom_quota']['#value'] * 1024 * 1024;
       break;

      // MB
      default:
        // leave as is
       break;
    }
  }

  // Validate the expiration time
  $matches = array();
  preg_match("/[^\-\d](\d+) *$/", $form['custom_expiration_date']['#value'], $matches);
  // No numeric found
  if(!array_key_exists(0, $matches)) {
    form_set_error('custom_expiration_date', t('Please enter a positive <b>integer</b> value as the expiration date'));
  }
  else {
    $form['custom_expiration_date']['#value'] = $matches[0];
  }
}

/**
 * Confirmation page of overwriting an existing entry (when the admin tries
 *   to add the same user to the quota table).
 *
 *
**/
function tripal_galaxy_confirm_overwrite_user_quota($form, &$form_state, $uid, $n_quota, $n_exp_date) {

  $form['#submit'][] = 'tripal_galaxy_admin_overwrite_user_quota';

  // Populate the table from the custom quota db table (users, quota, exp date).
   $header = array(
     'uid' => t('UID'),
     'user' => t('User'),
     'custom_quota' => t('Custom Quota'),
     'exp_date' => t('Expiration Date'),
   );

  $old_row = array();
  $new_row = array();

  // Old (api call already returns the var as an array)
  $old_row = tripal_galaxy_gather_user_custom_quotas($uid);

  // New
  $new_row[] = array(
    'uid' => $uid,
    'user' => $old_row[0]['user'],
    'custom_quota' => $n_quota,
    'exp_date' => $n_exp_date,
  );

  // Title
  $form['confirm_title'] = array (
    '#type' => 'markup',
    '#markup' => '<h2>Attempting to add quota to existing user</h2>'
  );

  // Display old entry
  $form['old_entry_title'] = array (
  '#type' => 'markup',
  '#markup' => '<h4>Old Entry</h4>',
  );
  $form['old_entry'] = array (
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $old_row
  );

  // Display new entry
  $form['new_entry_title'] = array (
  '#type' => 'markup',
  '#markup' => '<h4>New Entry</h4>',
  );
  $form['new_entry'] = array (
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $new_row
  );

  // Confirmation form
  $form = confirm_form(
    $form,
    t('Confirm Overwrite'),
    // Return page if the admin cancels change
    '/admin/tripal/files',
    // Description
    '<p>' . t('This action cannot be undone') . '</p>',
    // Yes text.
    t('Overwrite'),
    // No text
    t('Cancel'),
    // Internal name for this confirm_form
    'confirm'
  );
  return $form;
}

/**
 * Overwrite a user's current quota entry with the new quota and/or exp_date
 *
 *
**/
function tripal_galaxy_admin_overwrite_user_quota($form, &$form_state) {

  // API call to overwrite
  $update_entry = array(
    'custom_quota' => $form['new_entry']['#rows'][0]['custom_quota'],
    'custom_expiration' => $form['new_entry']['#rows'][0]['exp_date'],
  );
  tripal_galaxy_update_user_quota($update_entry, $form['new_entry']['#rows'][0]['uid']);
  drupal_set_message(t('Entry updated in the custom quota'));
  drupal_goto('/admin/tripal/files');

}

/**
 * For adding a NEW user to the table
**/
function tripal_galaxy_add_user_custom_quota_submit($form, &$form_state) {
 // Add the entry to the db table via sql

 // Get the UID of the given user
 $uid = db_query("SELECT uid
                  FROM {users}
                  WHERE name = :name",
                  array(':name' => $form['find_user']['#value']))
                  ->fetchObject()->uid;

  $values = array(
    'uid' => (int)$uid,
    'custom_quota' => (int)$form['custom_quota']['#value'],
    'custom_expiration' => (int)$form['custom_expiration_date']['#value']
  );

  // API call
  tripal_galaxy_add_user_quota($values);

 drupal_set_message(t('User added to the custom quota'));
 drupal_goto('/admin/tripal/files');
}


/**
 * Edit an existing users' quota and/or expiration date
**/
function tripal_galaxy_edit_user_quota($form, &$form_state, $uid) {

  // Get the name of the user to edit
  $name = db_query("SELECT name
                   FROM {users}
                   WHERE uid = :uid",
                   array(':uid' => $uid))
                   ->fetchObject()->name;

  $form['uid'] = array(
    '#type' => 'hidden',
    '#value' => $uid
  );

  $form['user'] = array (
   '#type' => 'markup',
   '#markup' => "<h3>User</h3><p>$name</p>",
  );

  // Get the current quota values of the user
  $entry = db_query("SELECT *
                     FROM {tripal_galaxy_custom_quota}
                     WHERE uid = :uid",
                     array(':uid' => $uid))
                     ->fetchObject();

  // Custom quota textfield (prepopulated with current value)
  $form['custom_quota'] = array (
    '#type' => 'textfield',
    '#title' => 'Custom User Quota',
    '#description' => 'Set the maximum amount of space on the server that a  user can consume.
    <br></br>Suffixes include MB, GB, TB. Leaving the suffix blank means the suffix will be MB.',
    '#default_value' => $entry->custom_quota,
  );

  // Custom exp date textfield (prepopulated with current value)
  $form['custom_expiration_date'] = array (
    '#type' => 'textfield',
    '#title' => 'Custom User Expiration Date',
    '#description' => 'The number of days that a user uploaded file can remain on the server before it is automatically removed.',
    '#default_value' => $entry->custom_expiration,
  );

  // Submit button
  $form['update_user'] = array (
    '#type' => 'submit',
    '#value' => t('Update User'),
  );

    return $form;
}

/**
 * Same validate as the add user with the exception of no duplicate entry
**/
function tripal_galaxy_edit_user_quota_validate($form, &$form_state){

  // Check that the values of quota and expiration date are numberical and with
    // digital storage nomenclature

    // Validate the qutoa
    $matches = array();
    preg_match("/[^\-\d](\d+) ?(MB|GB|TB)?$/", $form['custom_quota']['#value'], $matches);
    // $matches[0] will be the numeric value
    // $matches[1] will be the digital storage size: MB, GB, TB, or blank
    //   (defaults to MB)
    // No numeric found
    if(!array_key_exists(0, $matches)) {
      form_set_error('custom_quota', t('Please enter a positive <b>integer</b> value as the quota'));
    }
    else {
      $form['custom_quota']['#value'] = $matches[0];
    }
    // if the 2nd element is occupied by a valid suffix we need to check to see
    // what we have to multiply the value by (1024 for GB 1024^2 for TB because
    // we assume that the initial number is already in MB)
    if(array_key_exists(1, $matches)) {
      switch ($matches[1]) {

        case 'GB':
          // Multiply $matches[1] by 1024 and overwrite
          // $form['custom_quota']['#value']
          $form['custom_quota']['#value'] = (int)$form['custom_quota']['#value'] * 1024;
          break;

        case 'TB':
        // Multiply $matches[1] by 1024^2 and overwrite
        // $form['custom_quota']['#value']
        $form['custom_quota']['#value'] = (int)$form['custom_quota']['#value'] * 1024 * 1024;
         break;

        // MB
        default:
          // leave as is
         break;
      }
    }

    // Validate the expiration time
    $matches = array();
    preg_match("/[^\-\d](\d+) *$/", $form['custom_expiration_date']['#value'], $matches);
    // No numeric found
    if(!array_key_exists(0, $matches)) {
      form_set_error('custom_expiration_date', t('Please enter a positive <b>integer</b> value as the expiration date'));
    }
    else {
      $form['custom_expiration_date']['#value'] = $matches[0];
    }

}

/**
 * Same submit as the quota overwrite function
**/
function tripal_galaxy_edit_user_quota_submit($form, &$form_state) {
  // API call to update
  $update_entry = array (
    'custom_quota' => (int)$form['custom_quota']['#value'],
    'custom_expiration' => (int)$form['custom_expiration_date']['#value'],
  );
  tripal_galaxy_update_user_quota($update_entry, $form['uid']['#value']);
  drupal_set_message(t('Entry updated in the custom quota'));
  drupal_goto('/admin/tripal/files');
}
