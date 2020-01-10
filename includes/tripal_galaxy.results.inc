<?php


/**
 * The report page for a galaxy workflow submission.
 *
 * @param int $sid
 *   The submission ID of the workflow.
 */
function tripal_galaxy_workflow_report(int $sid) {
  if (!$sid) {
    return FALSE;
  }
  if (!is_numeric($sid)) {
    return FALSE;
  }

  // Get the submission details.
  $submission = tripal_galaxy_get_submission($sid);
  if (!$submission) {
    return FALSE;
  }


  // If this a webform submission make sure the user has access to the
  // node that this workflow belongs to.
  if ($submission->nid) {
    $node = node_load($submission->nid);

    // Does this user have access to this node?
    if (!node_access('view', $node)) {
      drupal_access_denied();
      module_invoke_all('exit');
      drupal_exit();
    }
    $title = $node->title;
  }

  // Get the galaxy server.
  $galaxy = db_select('tripal_galaxy', 'tg')
  ->fields('tg')
  ->condition('galaxy_id', $submission->galaxy_id)
  ->execute()
  ->fetchObject();

  // Before doing anything check that the history has not been deleted.
  $galaxy_instance = tripal_galaxy_get_connection($submission->galaxy_id);
  $history_name = tripal_galaxy_get_history_name($submission);
  $error = [];
  $content = [];
  $history = tripal_galaxy_get_history($galaxy_instance, $history_name);

  if ($history['deleted'] === TRUE) {
    $content['deleted'] = [
        '#type' => 'item',
        '#title' => t('Results deleted from Galaxy Server'),
        '#description' => t('It appears this analysis result has been deleted from the galaxy server on which it was run. Please return to the previous page and delete this from your workflow listing.'),
    ];
    db_update('tripal_galaxy_workflow_submission')->fields([
        'status' => 'deleted',
    ])
    ->condition('sid', $submission->sid)
    ->execute();
    return $content;
  }

  // Get the list of input files so we can exclude them from the results
  $files_query = db_select('file_usage', 'fu');
  $files_query->join('file_managed', 'fm', 'fu.fid = fm.fid');
  $files_query->fields('fm', ['filename']);
  $files_query->condition('fu.id', $sid);
  $files_query->condition('fu.module', 'tripal_galaxy');
  $files_query->condition('fu.type', 'submission');
  $input_files = $files_query->execute()->fetchCol();


  // Add a warning message at the top if there are errors.
  if ($submission->status == 'Error') {
    $content['status_message'] = [
      '#type' => 'markup',
      '#markup' => '<div class="messages error">' . t('This analysis failed. Check for errors in the subission details or in the error section if result files are present.') . '</div>',
    ];
  }

  // Print the analysis name.
  $content['analysis_name'] = [
    '#type' => 'item',
    '#title' => 'Analysis Name',
    '#markup' => $title,
  ];

  // Add in a feildset for the submission details.
  $content['submission_details'] = tripal_galaxy_workflow_report_build_submission($submission, $galaxy);


  // Add a section now for errors, but it will only get shown if there
  // are errors.
  $content['errors'] = [
    '#type' => 'fieldset',
    '#title' => t('Errors'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#attributes' => [
      'class' => [
        'collapsible',
        'collapsed',
      ],
    ],
    '#attached' => [
      'js' => [
        'misc/collapse.js',
        'misc/form.js',
      ],
    ],
  ];


  // Get details about the galaxy server and the connection to Galaxy.
  $galaxy = tripal_galaxy_get_connection($submission->galaxy_id);

  // This is just a placeholder for the results text. It will get fully
  // set below.
  $content['results'] = [
      '#type' => 'item',
  ];

  $errors = [];
  $has_any_results = FALSE;
  if (is_array($submission->errors)) {

    // If the workflow status is 'Completed' then there will be a history and
    // history contents. We'll get those out and remove them so we
    // can more easily iterate through the steps.
    if (array_key_exists('history_contents', $submission->errors)) {
      $history_contents = $submission->errors['history_contents'];
    }
    if (array_key_exists('history_info', $submission->errors)) {
      $history_info = $submission->errors['history_info'];
    }

    if (isset($history_contents) and is_array($history_contents) and
        isset($history_info) and is_array($history_info)) {

      $has_any_results = TRUE;
      foreach ($history_contents as $index => $history_content) {

        // Skip history content items that should be hidden.
        if ($history_content['visible'] != TRUE) {
          continue;
        }

        // Don't show input files.
        if (in_array($history_content['name'], $input_files)) {
          continue;
        }

        $content_id = 'history_content_' . $history_content['id'] . '_' . $index;

        // Handle Dataset Collections.
        if ($history_content['history_content_type']  == 'dataset_collection') {
          $content[$content_id] = tripal_galaxy_workflow_report_build_result_dataset_collection(
              $history_content, $submission, $node, $galaxy);
        }
        // Handle Datasets.
        else {
          // If there are any errors on any file creation, then we'll add
          // the error message to our file_errors list.
          if ($history_content['state'] == 'error') {
            $errors[] = [
              [
                'data' => $history_content['name'],
                'header' => TRUE,
              ],
              '<pre>' . $history_content['creating_job']['stderr'] . '</pre>'
            ];
            continue;
          }
          $content[$content_id] = tripal_galaxy_workflow_report_build_result_dataset($history_content,
              $submission, $node, $galaxy);
        }
      }
    }
  }

  if (count($errors) > 0) {
    $content['errors']['table'] = [
      '#type' => 'markup',
      '#markup' => theme_table([
        'header' => [],
        'rows' => $errors,
        'attributes' => [
          'class' => 'tripal-galaxy-error-table'
        ],
        'sticky' => FALSE,
        'caption' => '',
        'colgroups' => [],
        'empty' => '',
      ]),
    ];
  }
  else {
    unset($content['errors']);
  }

  if ($has_any_results) {
    $content['results'] = [
        '#type' => 'item',
        '#title' => t('Result Files'),
        '#markup' => t('The result files are listed below. Click the filename to view or download results. Expand the field for the result below to
        view the status, peek or download result files.'),
    ];
  }
  else {
    $content['results'] = [
        '#type' => 'item',
        '#title' => t('Results'),
        '#markup' => t('Currently, no results exist for this job. The current state of the job is:  %state.', [
            '%state' => $submission->status,
        ]),
    ];
  }

  return $content;
}

/**
 * Builds the submission fieldset for the report page.
 */
function tripal_galaxy_workflow_report_build_submission($submission, $galaxy) {

  // Create a galaxy logo clickable image.
  $galaxy_logo = l('<img src="' . url(drupal_get_path('module', 'tripal_galaxy') . '/theme/images/PoweredByGalaxy120.png') . '" border="0"></img>', "https://galaxyproject.org/", [
    'html' => TRUE,
    'attributes' => [
      'target' => '_blank',
    ],
  ]);

  $title = $submission->workflow_name;

  $headers = [];
  $submission_details = [];
  $submission_details[] = [
      [
          'data' => 'Workflow Name',
          'header' => TRUE,
          'width' => '25%',
      ],
      $title,
  ];
  if (user_access('administer galaxy')) {
    $submission_details[] = [
        [
            'data' => 'Submission ID',
            'header' => TRUE,
            'width' => '25%',
        ],
        $submission->sid,
    ];
    $submission_details[] = [
        [
            'data' => 'Workflow ID',
            'header' => TRUE,
            'width' => '25%',
        ],
        $submission->workflow_id,
    ];
    $submission_details[] = [
        [
            'data' => 'Invocation ID',
            'header' => TRUE,
            'width' => '25%',
        ],
        $submission->invocation_id,
    ];
  }
  $submission_details[] = [
      [
          'data' => 'Status',
          'header' => TRUE,
      ],
      $submission->status,
  ];
  $submission_details[] = [
      [
          'data' => 'Submission Error',
          'header' => TRUE,
      ],
      array_key_exists('error', $submission->errors) ? $submission->errors['error'] : '',
  ];
  $submission_details[] = [
      [
          'data' => 'Submission Date',
          'header' => TRUE,
      ],
      format_date($submission->submit_date),
  ];
  $submission_details[] = [
      [
          'data' => 'History Name',
          'header' => TRUE,
      ],
      tripal_galaxy_get_history_name($submission),
  ];
  $submission_details[] = [
      [
          'data' => 'Start Time',
          'header' => TRUE,
      ],
      $submission->start_time ? format_date($submission->start_time) : '',
  ];
  $submission_details[] = [
      [
          'data' => 'End Time',
          'header' => TRUE,
      ],
      $submission->end_time ? format_date($submission->end_time) : '',
  ];

  $submission_details[] = [
      [
          'data' => 'Galaxy Server',
          'header' => TRUE,
      ],
      $galaxy->servername . '<br>' . $galaxy_logo,
  ];

  $submission_section = [
      '#type' => 'fieldset',
      '#title' => t('Submission Details'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#attributes' => [
        'class' => [
          'collapsible',
          'collapsed',
        ],
      ],
      '#attached' => [
        'js' => [
          'misc/collapse.js',
          'misc/form.js',
        ],
      ],
  ];

  $submission_section['table'] = [
    '#type' => 'markup',
    '#markup' => theme_table([
      'header' => $headers,
      'rows' => $submission_details,
      'attributes' => [
        'class' => 'tripal-galaxy-submission-table'
      ],
      'sticky' => FALSE,
      'caption' => '',
      'colgroups' => [],
      'empty' => '',
    ]),
  ];

  return $submission_section;
}
/**
 * Creates a result elemnt for a single file.
 */
function tripal_galaxy_workflow_report_build_result_dataset_collection($history_content, $submission, $node, $galaxy) {
  $step_content = [
    '#type' => 'fieldset',
    '#title' => $history_content['name'] . ' (data collection)',
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#attributes' => [
      'class' => ['collapsible', 'collapsed'],
    ],
    '#attached' => [
      'js' => [
          'misc/collapse.js',
          'misc/form.js',
      ],
    ],
  ];

  foreach ($history_content['elements'] as $element) {
    $step_content[$element['element_identifier']] = tripal_galaxy_workflow_report_build_result_dataset(
        $element['contents'], $submission, $node, $galaxy);
  }
  return $step_content;
}
/**
 * Creates a result elemnt for a single file.
 */
function tripal_galaxy_workflow_report_build_result_dataset($history_content, $submission, $node, $galaxy) {

  $step_content = [
    '#type' => 'fieldset',
    '#title' => $history_content['name'],
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#attributes' => [
      'class' => ['collapsible', 'collapsed'],
    ],
    '#attached' => [
      'js' => [
        'misc/collapse.js',
        'misc/form.js',
      ],
    ],
  ];

  // If this is a file add viewing and download links
  if ($history_content['type'] == 'file') {
//     if (!isset($history_content['content_link'])) {
//       continue;
//     }

    // If this is an HTML file then add a view link.
    if ($history_content['extension'] == 'html') {
      $dataset_id = $history_content['id'];
      $dataset = tripal_galaxy_get_dataset($submission, $dataset_id);
      $proxy_url = tripal_galaxy_get_proxy_url($submission, $dataset, $node->uid, 'download');
      $step_content['html_view'] = [
        '#type' => 'item',
        '#title' => t('Download web page report'),
        '#markup' => l($history_content['name'], $proxy_url, ['attributes' => ['target' => '_blank']]),
      ];
    }
    // If this is not an HTML file then provide link to view the file
    // unless it exceeds 1MB.  If Galaxy provides a peek then show that.
    else {
      $link = $history_content['content_link'];
      $dataset_id = $history_content['id'];
      $dataset = tripal_galaxy_get_dataset($submission, $dataset_id);
      $proxy_url = tripal_galaxy_get_proxy_url($submission, $dataset, $node->uid, 'download');
      $file_size = tripal_format_bytes($history_content['file_size']);
      $step_content['download_link'] = [
        '#type' => 'item',
        '#title' => t('Download File'),
        '#markup' => l($history_content['name'], $proxy_url) . ' (' . $file_size . ')',
      ];

      /*             // Any files that are smaller than 1MB can be shown in a browser.
       if ($history_content['file_size'] < pow(10, 6)) {
       // SPF: originally this was $history_content['dataset_id'] but for
       // some reason that value seems off for v17.09 as it does not
       // point to the correct dataset. Rather the 'id' does.  However,
       // in v18.01 it seems fixed and both 'id' and 'dataset_id' are the
       // same.
       $dataset_id = $history_content['id'];
       $dataset = tripal_galaxy_get_dataset($submission, $dataset_id);
       $proxy_url = tripal_galaxy_get_proxy_url($submission, $dataset, $node->uid, 'viewer-full');
       $step_content['html_view'] = [
       '#type' => 'item',
       '#title' => t('View File'),
       '#markup' => l($history_content['name'], $proxy_url, ['attributes' => ['target' => '_blank']] ),
       ];
       } */

      // Provide a peek if one is provided.
      if ($history_content['peek'] != NULL) {
        $step_content['peek'] = [
          '#type' => 'item',
          '#title' => 'Peek',
          '#description' => 'A short view of the first few lines of the output file.',
          '#markup' => '<div class="tripal-galaxy-file-peek">' . $history_content['peek'] . '</div>',
        ];
      }
    }
  }
  return $step_content;
}


/**
 * A generic full page for viewing content from Galaxy.
 *
 * @param string $proxy_id
 *   A unique proxy ID that maps to a URL.
 */
function tripal_galaxy_stream_link_proxy(string $proxy_id) {
  if (array_key_exists($proxy_id, $_SESSION['tripal_galaxy_proxy_urls'])) {

    // Get the current proxy details, so we can map links for download.
    $cproxy = $_SESSION['tripal_galaxy_proxy_urls'][$proxy_id];
    $dataset = $cproxy['dataset'];
    $uid = $cproxy['uid'];
    $sid = $cproxy['sid'];

    // retrieve the pertinents needed to download the file.
    $submission = tripal_galaxy_get_submission($sid);
    $galaxy_id = $submission->workflow->galaxy_id;
    $galaxy = tripal_galaxy_get_connection($galaxy_id);
    $dataset_id = $dataset['id'];

    $url = $galaxy->getURL() . $dataset['download_url'];
    $headers = get_headers($url);
    foreach ($headers as $header) {
      $results = preg_split('/:/', $header, 2);
      if (count($results) == 2) {
        drupal_add_http_header($results[0], $results[1]);
      }
    }
    print tripal_galaxy_stream_url_proxy_url($url);

    exit();
  }
  else {
    $content['name'] = [
        '#type' => 'markup',
        '#markup' => t('This page has expired. Please reload the job results page for uploaded links.'),
    ];
    return $content;
  }
}

/**
 * A generic full page for viewing content from Galaxy.
 *
 * @param string $proxy_id
 *   A unique proxy ID that maps to a URL.
 */
function tripal_galaxy_results_viewer_full_page(string $proxy_id) {
  if (array_key_exists($proxy_id, $_SESSION['tripal_galaxy_proxy_urls'])) {
    $content = tripal_galaxy_stream_url_proxy($proxy_id);

    // Replace relative URLs to have the proxy.
    $url = url('galaxy/link/' . $proxy_id);
    $content = preg_replace('/href\s*=\s*[\'"](?!\s*http)(.*?)[\'"]/', 'href=' . $url . '?url=\1', $content);
    return $content;
  }
  else {
    $content['name'] = [
        '#type' => 'markup',
        '#markup' => t('This link has expired. Please reload the job results page for uploaded links.'),
    ];
    return $content;
  }
}

/**
 * A generic page for viewing content from Galaxy.
 *
 * @param string $proxy_id
 *   A unique proxy ID that maps to a URL.
 */
function tripal_galaxy_results_viewer_page(string $proxy_id) {
  if (array_key_exists($proxy_id, $_SESSION['tripal_galaxy_proxy_urls'])) {

    // Get the current proxy details, so we can map links for download.
    $cproxy = $_SESSION['tripal_galaxy_proxy_urls'][$proxy_id];
    $dataset = $cproxy['dataset'];
    $uid = $cproxy['uid'];
    $sid = $cproxy['sid'];

    $user = user_load($uid);

    // Set the breadcrumb.
    $breadcrumb = [];
    $breadcrumb[] = l(t('Home'), '<front>');
    $breadcrumb[] = l($user->name, 'user/' . $uid);
    $breadcrumb[] = l(t('Analyses'), 'user/' . $uid . 'galaxy-jobs');
    $breadcrumb[] = l(t('Analysis Results'), 'user/' . $uid . '/galaxy-jobs/'  . $sid);
    drupal_set_breadcrumb($breadcrumb);

    // retrieve the pertinents needed to download the file.
    $submission = tripal_galaxy_get_submission($sid);
    $galaxy_id = $submission->workflow->galaxy_id;
    $galaxy = tripal_galaxy_get_connection($galaxy_id);
    $dataset_id = $dataset['id'];
    $name = $dataset['name'];

    $content['sub_title'] = [
        '#type' => 'markup',
        '#markup' => '<h2>Viewing results for ' . $name . '</h2>',
    ];
    $content['name'] = [
        '#type' => 'markup',
        '#markup' => l(t('View full screen'), 'galaxy/viewer-full/' . $proxy_id),
    ];
    $content['viewer'] = [
        '#type' => 'item',
        '#markup' => '<iframe src="' . url('galaxy/viewer-full/' . $proxy_id) . '" id="tripal-galaxy-results-viewer-iframe"></iframe>',
    ];

    return $content;
  }
  else {
    $content['name'] = [
        '#type' => 'markup',
        '#markup' => t('This link has expired. Please reload the job results page for uploaded links.'),
    ];
    return $content;
  }
}

/**
 * A generic callback for downloading content from Galaxy.
 *
 * @param string $proxy_id
 *   A unique proxy ID that maps to a URL.
 */
function tripal_galaxy_results_download(string $proxy_id) {

  // Get the current proxy details, so we can map links for download.
  $cproxy = $_SESSION['tripal_galaxy_proxy_urls'][$proxy_id];
  $dataset = $cproxy['dataset'];
  $uid = $cproxy['uid'];
  $sid = $cproxy['sid'];

  // retrieve the pertinents needed to download the file.
  $submission = tripal_galaxy_get_submission($sid);
  $galaxy_id = $submission->workflow->galaxy_id;
  $galaxy = tripal_galaxy_get_connection($galaxy_id);
  $dataset_id = $dataset['id'];

  $url = $galaxy->getURL() . $dataset['download_url'] . '?to_ext=' . $dataset['extension'];

  $headers = get_headers($url);
  foreach ($headers as $header) {
    $results = preg_split('/:/', $header, 2);
    if (count($results) == 2) {
      drupal_add_http_header($results[0], $results[1]);
    }
  }
  print tripal_galaxy_stream_url_proxy_url($url);

  exit();
}


/**
 * Retrieves content from a Galaxy URL and stream it back.
 *
 * @param string $proxy_id
 *   A unique ID that maps to a URL.
 */
function tripal_galaxy_stream_url_proxy(string $proxy_id) {
  // Get the current proxy details, so we can map links for download.
  $cproxy = $_SESSION['tripal_galaxy_proxy_urls'][$proxy_id];
  $dataset = $cproxy['dataset'];
  $uid = $cproxy['uid'];
  $sid = $cproxy['sid'];

  // retrieve the pertinents needed to download the file.
  $submission = tripal_galaxy_get_submission($sid);
  $galaxy_id = $submission->workflow->galaxy_id;
  $galaxy = tripal_galaxy_get_connection($galaxy_id);
  $dataset_id = $dataset['id'];

  $url = $galaxy->getURL() . $dataset['download_url'];

  return tripal_galaxy_stream_url_proxy_url($url);
}

/**
 * Retrieves content from a Galaxy URL and stream it back.
 *
 * @param string $url
 *   A valid URL on the remote Galaxy server.
 */
function tripal_galaxy_stream_url_proxy_url(string $url) {
  $headers = get_headers($url);
  $ch = curl_init();

  curl_setopt($ch, CURLOPT_URL, $url);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
  $output = curl_exec($ch);
  if ($output === FALSE) {
    $error_msg = curl_error($ch);
    $this->error->setError('HTTP', $error_msg);
    return $error_msg;
  }
  curl_close($ch);

  if (!$output) {
    $output = 'The remote Galaxy server returns no display.';
  }
  return $output;
}


/**
 * Adds a URL to the proxy array.
 *
 * This function can be used to create a URL for downloading of a dataset from
 * a remote Galaxy server.
 *
 * @param stdClass $submission
 *   A submission object as obtained by the tripal_galaxy_get_submission()
 *   function.
 * @param array $dataset
 *   An array describing the dataset as returned by the
 *   tripal_galaxy_get_dataset() or tripal_galaxy_get_datasets() functions.
 * @param int $uid
 *   The user that is allowed to view this link.
 * @param string $action
 *   The action to perform when the link is clicked. Can be 'viewer',
 *   'viewer-full', or 'download'. The action 'viewer' will create a link for
 *   results to be shown inside of a page on the Drupal site. The action
 *   'viewer-full' will create a link for results to be shown stand-alone. The
 *   action 'download' will create a link that will start a download of the
 *   file.
 *
 * @throws Exception
 *
 * @return
 *   A URL that can be used, when clicked by the end-user, to either view or
 *   download the dataset.
 *
 * @ingroup tripal_galaxy_api
 */
function tripal_galaxy_get_proxy_url(stdClass $submission, array $dataset, int $uid, string $action = 'download') {

  if (!in_array($action, ['viewer', 'viewer-full', 'download'])) {
    throw new Exception('Please indicate the action as: "viewer" or "download".');
  }

  $galaxy_id = $submission->workflow->galaxy_id;
  $galaxy = tripal_galaxy_get_connection($galaxy_id);

  if (!array_key_exists('tripal_galaxy_proxy_urls', $_SESSION)) {
    $_SESSION['tripal_galaxy_proxy_urls'] = [];
  };
  $id = uniqid('TGPX', TRUE);
  $_SESSION['tripal_galaxy_proxy_urls'][$id]['dataset'] = $dataset;
  $_SESSION['tripal_galaxy_proxy_urls'][$id]['sid'] = $submission->sid;
  $_SESSION['tripal_galaxy_proxy_urls'][$id]['uid'] = $uid;

  // Create the link.
  $url = 'galaxy/' . $action . '/' . $id;
  return $url;
}