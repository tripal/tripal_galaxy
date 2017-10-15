<?php


/**
 * Provides the page with a list of files uploaded by the user.
 *
 * @param $uid
 *   The user ID.
 *
 * @return
 *   A Drupal render array.
 */
 function tripal_galaxy_user_files_page($uid) {

   // Get all of the files that have been uploaded by the user.
   // TODO: we should make this a paged query in case a user has a huge
   // numbef of uploaded files.
   $sql = "
     SELECT FM.fid, FM.filename, TGEF.expiration_date
     FROM {file_managed} FM
       INNER JOIN {file_usage} FU on FM.fid = FU.fid and FM.uid = :user_id
       LEFT JOIN {tripal_galaxy_expiration_files} TGEF on TGEF.fid = FU.fid
     WHERE FU.module = 'tripal_galaxy'
     GROUP BY FM.fid, TGEF.expiration_date
     ORDER BY FM.filename
   ";
   $files = db_query($sql, array(':user_id' => $uid));
   $rows = array();
   While ($entry = $files->fetchObject()) {
    $file = file_load($entry->fid);

    // Don't list files that don't exist on the file system.
    if (!file_exists($file->uri)) {
      continue;
    }

    $date_uploaded = date('Y-m-d H:i:s', $file->timestamp);
    $expiration = $entry->expiration_date ? date('Y-m-d H:i:s', $entry->expiration_date) : '';
    $actions = l('Delete', "user/$uid/galaxy-files/$file->fid/delete") . ' | ' .
               l('Renew', "user/$uid/galaxy-files/$file->fid/renew");

    $rows[] = array(
      $entry->fid,
      l($file->filename,"/user/$uid/galaxy-files/$file->fid"),
      $date_uploaded,
      $expiration,
      tripal_galaxy_format_bytes($file->filesize),
      $actions,
    );
  }
  $header = array('ID', 'File Name', 'Upload Date', 'Expiration', 'Size', 'Actions');

  // Get the user quota settings.
  $quota = tripal_galaxy_get_user_quota($uid);
  $usage = tripal_galaxy_get_user_usage($uid);

  $content = array(
    'page_title' => array(
      '#type' => 'markup',
      '#markup' => '<h2>Your Uploaded Files</h2>',
    ),
    'page_description' => array(
      '#type' => 'markup',
      '#markup' => '<p>' . t('Each user is allowed to consume a limited amount of space with uploaded files. This page provides details about your current usage, your limits and files you\'ve uploaded.') . '</p>',
    ),
    'usage' => array(
      '#type' => 'item',
      '#title' => 'Current Usage',
      '#markup' => tripal_galaxy_format_bytes($usage),
      '#description' => t('The total number of bytes you currently consume.'),
    ),
    'quota' => array(
      '#type' => 'item',
      '#title' => 'Current Quota',
      '#markup' => tripal_galaxy_format_bytes($quota->custom_quota),
      '#description' => t('The maximum number of bytes of files you can upload.')
    ),
    'expiration' => array(
      '#type' => 'item',
      '#title' => 'Current Days to Expire',
      '#markup' => $quota->custom_expiration,
      '#description' => t('The number of days a file will remain on the server before deltion. The expiration of date of a file can be renewed using the "Renew" link in the table below.')
    ),
    'file_list' => array(
      '#type' => 'item',
      '#title' => 'Uploaded Files',
      '#markup' => theme_table(array(
        'header' => $header,
        'rows' => $rows,
        'attributes' => array(),
        'caption' => t('Click a file name for more details.'),
        'colgroups' => array(),
        'sticky' => TRUE,
        'empty' => 'You currently have no uploaded files.',
      )),
    )
  );


  if ($usage < $quota->custom_quota) {
    drupal_set_message('Your account is currently under quota.');
  }
  else {
    drupal_set_message('Your account is currently over quota. Please remove some files before uploading more', 'warning');
  }

  return $content;
}

/**
 * User action to renew the expiration of a file.
 *
 * Adds the current time and the expiration date (either from default or if
 * the user has a custom expiration date) to tripal_galaxy_expiration_files
 * table.
 *
 **/
function tripal_galaxy_renew_file($fid) {
  $file = file_load($fid);

  // Get the file expiration information.
  $current = db_select('tripal_galaxy_expiration_files', 'tgfe')
    ->fields('tgfe')
    ->condition('fid', $fid)
    ->execute()
    ->fetchObject();

  // Search the custom quota table based off of uid to see if the user has a
  // custom quota setting.
  $quota = db_select('tripal_galaxy_custom_quota', 'tgcq')
    ->fields('tgcq')
    ->condition('uid', $file->uid)
    ->execute();

  // The db_update argument to update the expiration_date of the file
  $values = array(
    'fid' => $fid,
  );
  if($entry = $quota->fetchObject()) {
    // If there is an entry of this user add to their custom expiration as the
    // time to append to the current time() to update the expiration_date of
    // the file clicked
    $values['expiration_date'] = time() + $entry->custom_expiration;
  }
  // A custom expiration entry was not found so add the site wide default time
  // to the expiration date
  else {
    $values['expiration_date'] = time() + variable_get('tripal_galaxy_default_file_expiration', 60);
  }

  // Write the update to the db_table
  if ($current) {
    db_update('tripal_galaxy_expiration_files')
      ->fields($values)
      ->condition('fid', $fid)
      ->execute();
    drupal_set_message('Successfully updated expiration date.');
  }
  else {
    db_insert('tripal_galaxy_expiration_files')
      ->fields($values)
      ->execute();
    drupal_set_message('Successfully set the expiration date.');
  }

  drupal_goto('user/' . $file->uid . '/galaxy-files/');
}

/**
 * Downloads a file.
 *
 * @param $fid
 *   The File ID of the file to be downloaded.
 */
function tripal_galaxy_download_file($fid) {
  $file = file_load($fid);
  if (file_exists($file->uri)) {
    $headers = array();
    $headers['Content-Type'] = $file->filemime;
    $headers['Content-Disposition']  = 'attachment; filename=' . $file->filename;
    $headers['Content-Length'] = $file->filesize;
    file_transfer($file->uri, $headers);
  }
  else {
    drupal_set_message('Can not download. The file no longer exists on the server.', 'error');
    drupal_goto('user/' . $file->uid . '/galaxy-files/');
  }
}

/**
 * Provides a confirmation form for deleting a galaxy workflow uploaded file.
 */
function tripal_galaxy_delete_file_form($form, $form_state, $uid, $fid) {
  $form = array();

  $file = file_load($fid);

  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $uid,
  );
  $form['fid'] = array(
    '#type' => 'value',
    '#value' => $fid,
  );

  return confirm_form($form,
      t('Confirm deletion of the file named "' . $file->filename . '"?'),
      'user/' . $uid . '/galaxy-files/',
      t('Warning.  If this file is intended to be used with a analysis workflow submission that has not yet started then the workflow will fail. Once deleted, the file can no longer be used for new workflow submissions without uploading again.')
  );
}

/**
 * Implements a form submit for deleting a galaxy workflow uploaded file.
 */
function tripal_galaxy_delete_file_form_submit($form, &$form_state) {
  $fid = $form_state['values']['fid'];
  $uid = $form_state['values']['uid'];
  $file = file_load($fid);

  // Remove the file from the file_usage table for all entries that link
  // to the tripal_galaxy module.
  file_usage_delete($file, 'tripal_galaxy', NULL, NULL, 0);

  // Get any remaining usage for other modules
  $file_usage = file_usage_list($file);

  // If this file is still used by the tripal_galaxy module then something
  // didn't work right.
  if (in_array('tripal_galaxy', $file_usage)) {
    drupal_set_message('The file could not be removed.  Please contact the site administrator.', 'error');
  }

  // If there is no other usage of this file from other modules then delete it.
  if (count(array_keys($file_usage)) == 0) {
    if (file_unmanaged_delete($file->uri)) {

      // Also remove the md5 checksum.
      if (file_exists(file_unmanaged_delete($file->uri . '.md5'))) {
        file_unmanaged_delete($file->uri . '.md5');
      }
      drupal_set_message('The file has been fully removed.');
    }
    else {
      drupal_set_message('The file has removed from this list and does not count against your quota, but other components of this site rely on this file. Thus it has not been fully removed.');
    }
  }
  drupal_goto('user/' . $file->uid . '/galaxy-files/');
}

/**
 * Provides details about a file.
 */
function tripal_galaxy_view_file($uid, $fid) {
  $file = file_load($fid);

  $headers = array();
  $rows = array();

  $actions = l('Delete', "user/$uid/galaxy-files/$file->fid/delete") . '<br>' .
             l('Download', "user/$uid/galaxy-files/$file->fid/download");

  // Name row
  $rows[] = array(
    array(
      'data' => 'File Name',
      'header' => TRUE,
      'width' => '20%',
    ),
    $file->filename
  );

  $date_uploaded = date('Y-m-d H:i:s', $file->timestamp);
  $rows[] = array(
    array(
      'data' => 'Upload Date',
      'header' => TRUE,
      'width' => '20%',
    ),
    $date_uploaded
  );

  $expiration_date = db_select('tripal_galaxy_expiration_files', 'tgef')
    ->fields('tgef', array('expiration_date'))
    ->condition('fid', $fid)
    ->execute()
    ->fetchField();
  $expiration = $expiration_date ? date('Y-m-d H:i:s', $expiration_date) : '';
  $rows[] = array(
    array(
      'data' => 'Expiration Date',
      'header' => TRUE,
      'width' => '20%',
    ),
    $expiration
  );

  // Find which workflow submissions are using this file.
  $usage = file_usage_list($file);
  $usage = $usage['tripal_galaxy'];
  $workflow_nids = array();
  foreach ($usage as $step => $nid) {
    $nid = array_keys($nid)[0];
    if (!in_array($nid, $workflow_nids)) {
      $workflow_nids[] = $nid;
    }
  }
  $wf_links = array();
  foreach ($workflow_nids as $i => $nid) {
    $query = db_select('tripal_galaxy_workflow', 'tgw');
    $query->fields('tgw', array('workflow_name'));
    $query->innerJoin('tripal_galaxy_workflow_submission', 'tgws', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
    $query->fields('tgws', array('sid'));
    $query->condition('tgw.nid', $nid, '=');
    $results = $query->execute();
    $workflow = $results->fetchObject();
    if ($workflow) {
      $wf_links[] = l($workflow->workflow_name . ' (submission ID: ' . $workflow->sid . ')', "user/$uid/galaxy-jobs/$workflow->sid");
    }
  }
  $rows[] = array(
    array(
      'data' => 'Usage',
      'header' => TRUE,
      'width' => '20%',
    ),
    theme_item_list(array(
      'items' => $wf_links,
      'title' => '',
      'type' => 'ul',
      'attributes' => array(),
    )),
  );
  $rows[] = array(
    array(
      'data' => 'Actions',
      'header' => TRUE,
      'width' => '20%',
    ),
    $actions
  );

  $content = array(
    'description' => array(
      '#type' => 'markup',
      '#markup' => '<p>' . t('The following file has been uploaded for use in an analytical workflow.') . '</p>',
    ),
    'return' => array(
      '#type' => 'markup',
      '#markup' => '<p>' . l('View all Uploaded Files', "user/$uid/galaxy-files") . '</p>',
    ),
    'file_details' => array(
      '#type' => 'markup',
      '#markup' => theme_table(array(
        'header' => $headers,
        'rows' => $rows,
        'attributes' => array(),
        'sticky' => FALSE,
        'caption' => '',
        'colgroups' => array(),
        'empty' => '',
      )),
    ),
  );
  return $content;
}

/**
 * Retrieves the workflow submission report for the submitting user.
 *
 * @param $sid
 */
function tripal_galaxy_submission_user_report($sid) {
  global $user;

  // Set the breadcrumb.
  $breadcrumb = array();
  $breadcrumb[] = l('Home', '<front>');
  $breadcrumb[] = l($user->name, 'user/' . $user->uid);
  $breadcrumb[] = l('Analysis Results', 'user/' . $user->uid . '/galaxy-jobs');
  drupal_set_breadcrumb($breadcrumb);

  module_load_include('inc', 'tripal_galaxy', 'includes/tripal_galaxy.admin');

  return tripal_galaxy_workflow_report($sid);
}
