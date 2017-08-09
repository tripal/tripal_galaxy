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
 function tripal_galaxy_files_page($uid) {

   // Get all of the files that have been uploaded by the user.
   $sql = "
     SELECT FM.fid, TGEF.expiration_date, array_agg(FU.id) as workflow_nids
     FROM {file_managed} FM
       INNER JOIN {file_usage} FU on FM.fid = FU.fid and FM.uid = :user_id
       LEFT JOIN {tripal_galaxy_expiration_files} TGEF on TGEF.fid = FU.fid
     WHERE FU.module = 'tripal_galaxy'
     GROUP BY FM.fid, TGEF.expiration_date
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
    $workflow_nids = preg_replace('/[\{\}]/', '', $entry->workflow_nids);
    $workflow_nids = explode(',', $workflow_nids);

    // Add the workflow as a link to file_usage
    // Place the links of this particular file into the file_usage column
    $wf_links = '';
    foreach ($workflow_nids as $nid) {
      $query = db_select('tripal_galaxy_workflow', 'tgw');
      $query->fields('tgw', array('workflow_name'));
      $query->innerJoin('tripal_galaxy_workflow_submission', 'tgws', 'tgw.galaxy_workflow_id = tgws.galaxy_workflow_id');
      $query->fields('tgws', array('sid'));
      $query->condition('nid', $nid, '=');
      $results = $query->execute();
      $workflow = $results->fetchObject();
      if ($workflow) {
        $wf_links .= l($workflow->workflow_name . ' (' . $workflow->sid . ')', "user/$uid/galaxy-jobs/$workflow->sid") . ' ';
      }
    }

    $rows[] = array(
      'fid' => $entry->fid,
      'filename' => l($file->filename,"/user/$uid/galaxy-files/$file->fid/download"),
      'timestamp' => $date_uploaded,
      'expiration_date' => $expiration,
      'filesize' => tripal_galaxy_format_bytes($file->filesize),
      'file_usage' => $wf_links,
      'actions' => l('Delete', "/user/$uid/galaxy-files/$file->fid/delete") . ' ' . l('Renew', "/user/$uid/galaxy-files/$file->fid/renew")
    );
  }
  return $rows;
}

/**
 * User action to renew the expiration of a file.
 *
 * Adds the current time and the expiration date (either from default or if
 * the user has a custom expiration date) to tripal_galaxy_file_expiration
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
    // TODO: add code here for downloading file.....
  }
  else {
    drupal_set_message('Cannot download. The file no longer exists on the server.', 'error');
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

