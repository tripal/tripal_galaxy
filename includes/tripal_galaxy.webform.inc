<?php

/**
 * @file
 * Imports a workflow from a remote Galaxy server and generates a webform.
 */

/**
 * The base function to submitted_list()
 */
function tripal_galaxy_build_webform($galaxy_id, $workflow_id) {

  // We need to keep track of data_input (and data_collection_inputs) so that
  // we can map allowed extensions from tools to these input steps.
  $data_inputs = [];

  // An object that will contain the componentns and conditions that
  // will be used to build the webform.
  $webform = new stdClass();
  $webform->components = [];
  $webform->conditionals = [];

  // Get a connection to Galaxy.
  $galaxy = tripal_galaxy_get_connection($galaxy_id);
  if (!$galaxy) {
    $error = $galaxy->getError();
    drupal_set_message('Could not connect to Galaxy server. Galaxy Server response: ' . $error['message'], 'error');
    return FALSE;
  }

  // Get the workflow from Galaxy.
  $gworkflows = new GalaxyWorkflows($galaxy);
  $workflow = $gworkflows->show([
    'workflow_id' => $workflow_id,
  ]);
  if (!$workflow) {
    $error = $galaxy->getError();
    drupal_set_message('Error retrieving workflow. Galaxy Server response: ' . $error['message'], 'error');
    return FALSE;
  }

  // Create a history which will be used for building tools. We have to build
  // the tools so that we can get all of the inputs and their default values.
  $history = tripal_galaxy_create_history($galaxy, 'TG-import-tools');
  if (!$history) {
    drupal_set_message('Error retrieving tool build history. Galaxy Server response: ' . $error['message'], 'error');
    return FALSE;
  }

  // Add in the instruction page to the webform first.
  tripal_galaxy_build_webform_add_instruction_step($webform, $galaxy, $workflow_id);

  // Iterate through each step of the workflow.
  $tool_list = [];
  $steps = $workflow['steps'];
  //dpm($steps);

  foreach ($steps as $step_index => $step) {
    if (isset($step['annotation'])) {
      $step_annotation = $step['annotation'];

      // Add a markup component within a fieldset to display annotation from
      // each step from galaxy workflow.
      $markup_fieldset_cid = count($webform->components) + 1;
      $webform->components[] = [
        'cid' => $markup_fieldset_cid,
        'pid' => 0,
        'form_key' => "Step_" . $step_index . "_annotation_fieldset",
        'name' => 'Step Instructions',
        'type' => 'fieldset',
        'value' => '',
        'extra' => [
          'description' => '',
          'description_above' => 1,
          'private' => 0,
          'css_classes' => 'tripal-galaxy-fieldset',
          'title_display' => 0,
          'collapsible' => 1,
          'collapsed' => 0,
        ],
        'required' => '0',
        'weight' => $markup_fieldset_cid,
      ];
      $webform->components[] = [
        'cid' => $markup_fieldset_cid + 1,
        'pid' => $markup_fieldset_cid,
        'form_key' => "Step_" . $step_index . "_annotation_markup",
        'name' => 'workflow step annotation',
        'type' => 'markup',
        'value' => $step_annotation,
        'extra' => [
          'private' => 0,
          'format' => 'full_html',
          'display_on' => 'both',
        ],
        'required' => '0',
        'weight' => $markup_fieldset_cid + 1,
      ];
    }

    // Each step is contained in a fieldset. We'll name the field set after
    // it's step and if a tool is present we'll include the tool info.
    $cid = count($webform->components) + 1;
    $fieldset_key = "Step_" . $step_index . "_fieldset";
    $fieldset_name = 'Step ' . ($step_index + 2);

    // Add the fieldset to the components list.
    $webform->components[] = [
      'cid' => $cid,
      'pid' => '0',
      'form_key' => $fieldset_key,
      'name' => $fieldset_name,
      'type' => 'fieldset',
      'value' => '',
      'extra' => [
        'description' => '',
        'description_above' => 1,
        'private' => 0,
        'css_classes' => 'tripal-galaxy-fieldset',
        'title_display' => 0,
        'collapsible' => 1,
        'collapsed' => 0,
      ],
      'required' => '0',
      'weight' => $cid,
    ];

    $fieldset_cid = count($webform->components);

    // Handle the step type. So far we only know about 'data_input' and 'tool'.
    // The succeeding tool to the.
    switch ($step['type']) {

      // If this is for a data 'file' input.
      case 'data_input':
        $tool_input_name = array_key_exists('name', $step['tool_inputs']) ? $step['tool_inputs']['name'] : 'Data File';
        $tool_input_name = 'Data File';
        $tool_input_name = $workflow['inputs'][$step_index]['label'];

        // Make a fake tool and input so we can add the component for a
        // data_input step.
        $tool = [
          'id' => NULL,
          'name' => $tool_input_name,
        ];
        if ($tool_input_name) {
          $fieldset_name .= ': ' . $tool_input_name;
        }
        else {
          $fieldset_name .= ': Select Data';
        }
        $webform->components[$fieldset_cid - 1]['name'] = $fieldset_name;

        $input = [
          'name' => $tool_input_name,
          'full_name' => $tool_input_name,
          'type' => 'data_input',
        ];
        $input_id = tripal_galaxy_build_webform_add_component($webform, $workflow, $step, $tool, $input, $cid, $data_inputs);
        $data_inputs[$step_index] = $input_id;
        break;

      case 'data_collection_input':
        $tool_input_name = array_key_exists('name', $step['tool_inputs']) ? $step['tool_inputs']['name'] : 'Data Collection';

        $tool = [
          'id' => NULL,
          'name' => $tool_input_name,
        ];

        // We want to change the name of the fieldset to have the name of
        // the tool.
        if ($tool_input_name) {
          $fieldset_name .= ': ' . $tool_input_name;
        }
        elseif ($step['tool_inputs']['collection_type'] == 'list:paired') {
          $fieldset_name .= ': Select a Paired Data Set';
        }
        elseif ($step['tool_inputs']['collection_type'] == 'list') {
          $fieldset_name .= ': Select a Data Set';
        }
        $webform->components[$fieldset_cid - 1]['name'] = $fieldset_name;

        $input = [
          'name' => $tool_input_name,
          'full_name' => $tool_input_name,
          'type' => 'data_collection_input',
          'collection_type' => $step['tool_inputs']['collection_type'],
        ];
        $input_id = tripal_galaxy_build_webform_add_component($webform, $workflow, $step, $tool, $input, $cid, $data_inputs);
        $data_inputs[$step_index] = $input_id;
        break;

      // If this is a tool step then handle it.
      case 'tool':
        // Get the tool details from the Galaxy server.
        $tool_id = $step['tool_id'];
        $gtools = new GalaxyTools($galaxy);
        $tool = $gtools->build([
          'tool_id' => $tool_id,
          'history_id' => $history['id'],
        ]);
        if (!$tool) {
          $error = $galaxy->getError();
          drupal_set_message('Error building tool: ' . $tool_id . '. Galaxy Server response: ' . $error['message'], 'error');
          return FALSE;
        }
        //dpm($tool);

        $tool_list[] = '<b>' . $tool['name'] . ' (version ' . $tool['version'] . ')</b>: ' . $tool['description'];
        // We want to change the name of the fieldset to have the name of
        // the tool.
        // we also want to add tool description to the end, like what we see in
        // Galaxy.
        $fieldset_name .= ': ' . $tool['name'] . ' (Galaxy Version ' . $tool['version'] . ')';
        $webform->components[$fieldset_cid - 1]['name'] = $fieldset_name;

        // Itearte through each of the inputs of this tool and add them
        // to the webform.
        foreach ($tool['inputs'] as $input) {
          $step = $workflow['steps'][$step_index];
          // We add a new 'full_name' key to the input so that we
          // can keep track of the nested name of an input. The nested
          // name is used for finding component defaults.
          $input['full_name'] = $input['name'];
          tripal_galaxy_build_webform_add_tool_input($webform, $workflow, $step, $tool, $input, $cid, $data_inputs);
        }

        break;

      default:
        drupal_set_message('Unhandled step type:  ' . $step['type'] . ".", 'error');
        return;
    } // end switch ($step['type']) { ...

    // Add a page break after each step except on the last one.
    if ($step_index < count($steps) - 1) {
      $webform->components[] = [
        'cid' => count($webform->components) + 1,
        'pid' => 0,
        // Add 3 to the step_index because the instruction appear on step 1
        // and this pagebreak introduces the next step.
        'name' => "Step " . ($step_index + 3),
        // 'name' => $tool ? $tool['name'] : "Step" . ($step_index + 1),.
        'form_key' => "step_" . ($step_index + 3) . "_pagebreak",
        'type' => 'pagebreak',
        'weight' => count($webform->components) + 1,
      ];
    }
  } // end foreach ($steps as $step_index => $step) { ...

  // Before creating a webform node we want to sort the conditionals.
  usort($webform->conditionals, 'tripal_galaxy_build_webform_sort_conditionals');

  // Add the tool list to our instructions step.
  $tool_list = theme_item_list([
    'items' => array_unique($tool_list),
    'type' => 'ul',
    'title' => 'This workflow uses the following tools:',
    'attributes' => [],
  ]);
  $webform->components[1]['value'] .= $tool_list;

  // How do I extend the node type so it tells me in the form id that this
  // is a 'galaxy' webform?
  $node = new stdClass();
  $node->type = 'webform';
  node_object_prepare($node);
  $node->title = $workflow['name'];
  $node->language = 'en';
  $node->body[LANGUAGE_NONE][0]['value'] = '';
  $node->body[LANGUAGE_NONE][0]['format'] = 'full_html';
  $node->uid = 1;
  $node->promote = 0;
  $node->comment = 0;

  // Attach the webform to the node.
  $node->webform = [
    'confirmation' => '',
    'confirmation_format' => NULL,
    'redirect_url' => '',
    'status' => '1',
    'block' => '0',
    'teaser' => '1',
    'allow_draft' => '0',
    'auto_save' => '0',
    'submit_notice' => '1',
    'confidential' => '0',
    'submit_text' => '',
    'submit_limit' => '-1',
    'submit_interval' => '-1',
    'total_submit_limit' => '-1',
    'total_submit_interval' => '-1',
    'progressbar_bar' => '1',
    'progressbar_page_number' => '1',
    'progressbar_percent' => '1',
    'progressbar_pagebreak_labels' => '1',
    'progressbar_include_confirmation' => '1',
    'progressbar_label_first' => 'Step 1',
    'progressbar_label_confirmation' => 'Submit',
    'preview' => '1',
    'preview_next_button_label' => 'Next Step',
    'preview_prev_button_label' => 'Previous Step',
    'preview_title' => '',
    'preview_message' => '',
    'preview_message_format' => NULL,
    'record_exists' => TRUE,
    'roles' => [
      '1',
      '2',
    ],
    'emails' => [],
    'components' => $webform->components,
    'conditionals' => $webform->conditionals,
  ];

  // Save the node.
  node_save($node);
  // A new node will have been created be sure to record the nid in the
  // workflow table on the db.
  $nid = (int) $node->nid;
  return $nid;
}

/**
 * Adds the instructions component to the webform.
 */
function tripal_galaxy_build_webform_add_instruction_step($webform, $galaxy, $workflow_id) {
  $webform->components[] = [
    'cid' => 1,
    'pid' => 0,
    'form_key' => 'galaxy_webform',
    'name' => 'Step 1: Instructions',
    'type' => 'fieldset',
    'value' => '',
    'extra' => [
      'description_above' => 1,
      'private' => 0,
      'css_classes' => '',
      'title_display' => 1,
      'collapsible' => 1,
      'collapsed' => 0,
      'workflow_id' => $workflow_id,
    ],
    'required' => '0',
    'weight' => 0,
  ];

  // The array of components for the webform.
  $webform->components[] = [
    'cid' => 2,
    'pid' => 1,
    'form_key' => 'galaxy_webform',
    'name' => 'Step 1: Instructions',
    'type' => 'markup',
    'value' => '<p>' . t("Here you can submit an analysis for execution.  This analysis makes use of several bioinformatics tools with each tool provided as a separate step in this workflow. As you move from step to step in the form you can alter tool settings to control how the tool performs. Some tools may not have settings or use output from tools that appeared as a previous step. Optional settings may be provided. <p><p>Execution of this workflow is supported by !galaxy.</p>!img",
      [
        '!galaxy' => l(t('Galaxy'), 'https://galaxyproject.org/',
        [
          'attributes' => [
            'target' => '_hidden',
          ],
        ]),
        '!img' => '<img src="' . url(drupal_get_path('module', 'tripal_galaxy') . '/theme/images/PoweredByGalaxy200.png',
        [
          'absolute' => TRUE,
        ]) . '">',
      ]
    ),
    'extra' => [
      'format' => 'full_html',
      'display_on' => 'both',
    ],
    'required' => '0',
    'weight' => 0,
  ];
  // Add a page break.
  $webform->components[] = [
    'cid' => count($webform->components) + 1,
    'pid' => 0,
    'name' => "Step 2",
    'form_key' => "step_2_pagebreak",
    'type' => 'pagebreak',
    'weight' => count($webform->components) + 1,
  ];
}

/**
 * Sort webform conditionals first by source and then by target IDs.
 */
function tripal_galaxy_build_webform_sort_conditionals($a, $b) {
  if ($a['rules'][0]['source'] == $b['rules'][0]['source']) {
    if ($a['actions'][0]['target'] == $b['actions'][0]['target']) {
      return 0;
    }
    if ($a['actions'][0]['target'] < $b['actions'][0]['target']) {
      return -1;
    }
    else {
      return 1;
    }
  }
  if ($a['rules'][0]['source'] < $b['rules'][0]['source']) {
    return -1;
  }
  else {
    return 1;
  }
}

/**
 * Adds a single input component or recurses.
 *
 * A function to construct a web form component based on a given tool input Some
 * of these workflows will have its 'vanilla' inputs and others will have its
 * inputs already chosen (workflow might have been constructed on the galaxy
 * server or imported)
 */
function tripal_galaxy_build_webform_add_tool_input($webform, $workflow, $step, $tool, $tool_input, $pid = 0, $data_inputs = NULL) {
  $step_index = $step['id'];
  // We want to keep track of which inputs come from a repeat input type
  // because there can be multiple values at different indicies.
  $is_repeat = array_key_exists('is_repeat', $tool_input) ? $tool_input['is_repeat'] : FALSE;

  // Some tool inputs are nested. These include conditionals, repeats and
  // operators. Therefore we weant to check what kind of input we
  // are dealing with. If it's not one of those nested components then
  // we can add the component. Otherwise we have to deal with each one
  // specially.
  switch ($tool_input['type']) {
    case 'repeat':

      $inputs = $tool_input['inputs'];
      foreach ($inputs as $repeat_index => $repeat_input) {
        $repeat_input['full_name'] = $tool_input['name'] . '|' . $repeat_index . '|' . $repeat_input['name'];
        $repeat_input['is_repeat'] = TRUE;
        tripal_galaxy_build_webform_add_tool_input($webform, $workflow, $step, $tool, $repeat_input, $pid, $data_inputs);
      }
      break;

    case 'conditional':

      // We need to update the field name for the conditional so
      // we can find it's default value during recursion.
      $test_param = $tool_input['test_param'];
      $test_param['full_name'] = $tool_input['full_name'] . '|' . $test_param['name'];
      if ($is_repeat) {
        $test_param['is_repeat'] = TRUE;
      }
      $test_param['type'] = 'conditional_select';

      // Now add the test param as a component for the webform.
      tripal_galaxy_build_webform_add_component($webform, $workflow, $step, $tool, $test_param, $pid, $data_inputs, TRUE);

      // Each condition must have a "source" component that they refer too.
      // Our source component is the test param.
      $test_param_index = count($webform->components);
      $source_id = $webform->components[$test_param_index - 1]['cid'];

      // Each valid value of the test parameter (i.e. cases) will lead to
      // a different set of inputs that will be made available. We need
      // to add all of those input components and associate them with the
      // correct case for the test param.
      foreach ($tool_input['cases'] as $case_num => $case) {
        $conditional = [
          'andor' => NULL,
          'weight' => count($webform->components),
          'rules' => [
            [
              'source_type' => 'component',
              // The conditional's parent.
              'source' => $source_id,
              // This means the element will be listening for the 'value'
              // (see the next element in this array)
              'operator' => 'equal',
              // This is the option for which this conditional is activated.
              'value' => $case['value'],
            ],
          ],
          'actions' => [],
        ];

        // If this case has input components we need to add them. Iterate
        // through the case inputs and add a new component for each
        // one and a new conditional action.
        if ($case['inputs']) {
          foreach ($case['inputs'] as $cindex => $case_input) {

            // For each input we'll add an action to show the field.
            // Get the target so we have it after we return from the
            // recursive call below.
            $target_component_index = count($webform->components);

            // We need to update the field name for the conditional so
            // we can find it's default value during recursion.
            $case_input['full_name'] = $tool_input['full_name'] . '|' . $case_input['name'];
            if ($is_repeat) {
              $case_input['is_repeat'] = TRUE;
            }

            tripal_galaxy_build_webform_add_tool_input($webform, $workflow, $step, $tool, $case_input, $pid, $data_inputs);

            // Now add the action to the condition for this target.
            $target_id = $webform->components[$target_component_index]['cid'];
            $conditional['actions'][] = [
              'target_type' => 'component',
              'target' => $target_id,
              'action' => 'show',
            ];
          }
          $webform->conditionals[] = $conditional;
        }
      }
      break;

    // If we don't have one of the nested types, then we have an input
    // type that is a single component, so let's add it.
    default:
      $input_values = $workflow['steps'][$step_index]['tool_inputs'];
      tripal_galaxy_build_webform_add_component($webform, $workflow, $step, $tool, $tool_input, $pid, $data_inputs, FALSE);
      break;
  }
}

/**
 * Adds a single component to the webform.
 */
function tripal_galaxy_build_webform_add_component($webform, $workflow, $step, $tool, $input, $pid, $data_inputs, $fixed = FALSE) {

  // Get the input name.
  $input_name = $input['name'];
  if (array_key_exists('label', $input)) {
    $input_name = $input['label'];
  }
  else {
    if (array_key_exists('title', $input)) {
      $input_name = $input['title'];
    }
  }

  // Initiate variables that will be used for building the webform component.
  $is_list = 1;
  $webform_value = "";
  $webform_type = 'select';
  $is_required = 1;
  $extra_description = '';
  if (isset($input['help'])) {
    $extra_description = $input['help'];
  }
  $extra = [
    'title_display' => 'before',
    'description' => $extra_description,
    'description_above' => 0,
    'items' => '',
    'aslist' => $is_list,
    'input_name' => $input['name'],
    'input_full_name' => $input['full_name'],
    'step_index' => $step['id'],
    'tool_id' => $tool['id'],
    'input_type' => $input['type'],
    'is_repeat' => array_key_exists('is_repeat', $input) ? $input['is_repeat'] : FALSE,
  ];
  if ($tool) {
    $extra['tool_name'] = $tool['name'];
  }

  // Get the value of this component input.
  $webform_value = tripal_galaxy_build_webform_get_input_value($workflow, $step['id'], $input['full_name']);

  // Is this a required component?
  if (array_key_exists('optional', $input) and $input['optional']) {
    $is_required = 0;
  }

  // Now, set the webform component type and the value depending on the
  // tool input type.
  switch ($input['type']) {
    case 'text':
      // Text should be the webform equivalent of textfield.
      $webform_type = 'textfield';
      break;

    case 'conditional_select':
      // A select box is fixed on conditionals that have been set by the
      // workflow creator. So, we want to show the selected option, and not
      // let the change it.
      $webform_type = 'fixed_value';
      $extra['description'] = 'This option has been preset for this workfow and cannot be changed.';

      // For the 'fixed_value' content type we want to set a human-readable
      // value.
      if (array_key_exists('options', $input)) {
        foreach ($input['options'] as $key => $val) {
          if ($val[1]) {
            if ($val[1] == $webform_value) {
              $extra['markup'] = $val[0];
            }
            // The val is now that sub-array containing the human readable
            // option
            // (index 0) as well as the unique key-name (index 1).
            if ($val[1]) {
              $extra['items'] .= $val[1] . "|" . $val[0] . "\n";
            }
            else {
              $extra['items'] .= "--empty--|" . $val[0] . "\n";
            }
          }
        }
      }
      break;

    case 'select':
      $webform_type = 'select';
      // Select should be the default list drop-down. Unless it has a display
      // of Checkbox.
      if ($input['multiple'] == TRUE) {
        $extra['multiple'] = TRUE;
        $extra['description'] = 'You may select as many options as desired.';
      }
      if (array_key_exists('options', $input)) {
        foreach ($input['options'] as $key => $val) {
          // The val is now that sub-array containing the human readable option
          // (index 0) as well as the unique key-name (index 1).
          if ($val[1]) {
            $extra['items'] .= $val[1] . "|" . $val[0] . "\n";
          }
          else {
            $extra['items'] .= "--empty--|" . $val[0] . "\n";
          }
        }
      }
      break;

    case 'data_column':
      $webform_type = 'select';
      // Data_column should be the default list drop-down.
      $extra['multiple'] = 0;
      break;

    case 'data_input':
      // The 'data_input' type is really not a Galaxy tool type. It's the type
      // we created for the 'data_input' step that doesn't have a tool.
      // we want to use this same function to create components so we fake
      // a tool and input and use this type.
      $webform_type = 'galaxy_sfile';

      // Intialize the allowed extension variables. These get set in the
      // appropraite 'data' tool type.
      $extra['filtering']['types'] = [];
      // The webform 'file' component wants a size. Let this default to the
      // server settings.
      $extra['filtering']['size'] = '0';
      // Set the allowed extension as per the rules on Galaxy.
      $extra['filtering']['addextensions'] = '';
      break;

    case 'data':
      // Data appears to be a refer to a file that exists in the user's history.
      $webform_type = 'select';
      $extra['filtering']['types'] = [
        '',
      ];
      // The webform 'file' component wants a size. Let this default to the
      // server settings.
      $extra['filtering']['size'] = '0';
      // Set the allowed extension as per the rules on Galaxy.
      if ($input['extensions']) {
        $extra['filtering']['types'] = $input['extensions'];
        $extra['filtering']['addextensions'] = implode(", ", array_values($input['extensions']));
      }

      // We need to find the upstream data_input component and copy the
      // allowed extensions there as well for the webform component to work
      // properly.
      $found_upstream = tripal_galaxy_build_webform_set_input_extensions($workflow, $input, $data_inputs, $webform);

      // If there is an upstream data collection then all is good and we can
      // use the input from that. Otherwise, we need to make this a data
      // input element so the user can supply a file.
      if (!$found_upstream) {
        $webform_type = 'galaxy_sfile';
      }

      break;

    case 'data_collection_input':
      // The 'data_collection_input' type is really not a Galaxy tool type.
      // It's the type we created for the 'data_collection_input' step that
      // doesn't have a tool. we want to use this same function to create
      // components so we fake a tool and input and use this type.
      switch ($input['collection_type']) {
        case 'list':
          $webform_type = 'galaxy_sflist';
          break;

        case 'paired':
          $webform_type = 'galaxy_pfile';
          break;

        case 'list:paired':
          $webform_type = 'galaxy_pflist';
          break;
      }
      // Intialize the allowed extension variables. These get set in the
      // appropraite 'data' tool type.
      $extra['filtering']['types'] = [];
      // The webform 'file' component wants a size. Let this default to the
      // server settings.
      $extra['filtering']['size'] = '0';
      // Set the allowed extension as per the rules on Galaxy.
      $extra['filtering']['addextensions'] = '';
      break;

    case 'data_collection':
      // Data collection type refers to an existing data collection in the
      // history.
      $webform_type = 'select';
      $extra['filtering']['types'] = [
        '',
      ];
      // The webform 'file' component wants a size. Let this default to the
      // server settings.
      $extra['filtering']['size'] = '0';
      // Set the allowed extension as per the rules on Galaxy.
      if ($input['extensions']) {
        $extra['filtering']['types'] = $input['extensions'];
        $extra['filtering']['addextensions'] = implode(", ", $input['extensions']);
      }

      // We need to find the upstream data_input component and copy the
      // allowed extensions there as well for the webform component to work
      // properly.
      tripal_galaxy_build_webform_set_input_extensions($workflow, $input, $data_inputs, $webform);
      break;

    case 'integer':
      $webform_type = 'number';
      $webform_value = $webform_value;
      break;

    case 'float':
      $webform_type = 'number';
      $webform_value = $webform_value;
      break;

    case 'boolean':
      $webform_type = 'select';
      if ($webform_value == 'true') {
        $webform_value = 'yes';
      }
      else {
        $webform_value = 'no';
      }
      // Boolean should have a single true/false value. It is also likely to
      // not have 'options' so we must invent our own (galaxy uses 'no' as The
      // false value and 'yes' as the true value)
      $extra['items'] = "yes|Yes\nno|No";
      $extra['aslist'] = 0;

      // Check defaults.
      break;

    case 'hidden_data':
      break;

    case 'hidden':
      break;

    // This looks like a field set in the drupal world so make it so, then
    // this part needs to be traversed to get all of the children elements.
    case 'section':
      // Add the section fieldset.
      $fieldset_key = $pid . '_' . $input['name'] . "_fieldset";
      $dcid = count($webform->components) + 1;
      $webform->components[] = [
        'cid' => $dcid,
        'pid' => $pid,
        'form_key' => $fieldset_key,
        'name' => $input['title'],
        'type' => 'fieldset',
        'value' => '',
        'extra' => [
          'description_above' => 1,
          'private' => 0,
          'css_classes' => '',
          'title_display' => 1,
          'collapsible' => 1,
          'collapsed' => 1,
        ],
        'required' => '0',
        'weight' => '1000',
      ];
      // Make this a parent component that holds the children values and call
      // add component again.
      $section_inputs = $input['inputs'];
      foreach ($section_inputs as $section_index => $section_input) {
        $section_input['full_name'] = $input['name'] . '|' . $section_input['name'];
        tripal_galaxy_build_webform_add_tool_input($webform, $workflow, $step, $tool, $section_input, $dcid, $data_inputs);
      }
      // phpcs:disable Squiz.ControlStructures.SwitchDeclaration.SpacingAfterBreak
      // phpcs:disable Squiz.PHP.NonExecutableCode.Unreachable
      return;
    break;
      // phpcs:enable Squiz.PHP.NonExecutableCode.Unreachable
      // phpcs:enable Squiz.ControlStructures.SwitchDeclaration.SpacingAfterBreak

    default:
      drupal_set_message('Unhandled tool input type: ' . $input['type'] . ".", 'error');
      break;
  }

  // We want to handle BDSS uniquely.
  if ($tool and $tool['id'] == 'bdss') {
    $input_name = 'Step ' . $step['id'] . ' Data Input';
    $webform_type = 'BDSS_file';
  }

  // If the input step uses an output from another tool then we need
  // to set that and not let the user change it.
  $linked = FALSE;
  if (array_key_exists($input['full_name'], $step['input_steps'])) {
    $source_step = $step['input_steps'][$input['full_name']]['source_step'];
    $step_output = $step['input_steps'][$input['full_name']]['step_output'];
    $webform_value = $source_step . '|' . $step_output;
    $webform_type = 'fixed_value';
    $extra['linked'] = TRUE;
    $linked = TRUE;
  }

  $cid = count($webform->components) + 1;
  $component = [
    'cid' => $cid,
    'pid' => $pid,
    'name' => $input_name,
    // This form_key has to be unique from ANYTHING else, otherwise the
    // component will not be presented.
    'form_key' => "step_" . ($step['id'] + 2) . '_' . $cid,
    'type' => $webform_type,
    'value' => $webform_value,
    'extra' => $extra,
    'weight' => $cid,
    'required' => $is_required,
  ];
  $webform->components[] = $component;

  return $cid;
}

/**
 * Sets allowed file extensions.
 *
 * Adds the allowed extensions to data_input and data_collection_input tools.
 * The data_input and data_collection_input "tools" don't have any allowed file
 * types. The allowed file types are set in the tool that uses them as input.
 * Therefore, we need to traverse the steps, find those that use the data_input
 * and data_collection_input as an input and set the allowed types.
 *
 * @param array $workflow
 *   The Galaxy workflow array.
 * @param array $input
 *   The tool input from the tool that will be added as a component.
 * @param array $data_inputs
 *   The array that maps the data_input steps to the component IDs.
 * @param $webform
 *   The webform object containing conditionals and components that can be
 *   added to.
 */
function tripal_galaxy_build_webform_set_input_extensions(array $workflow, array $input, array $data_inputs, &$webform) {
  $full_name = $input['full_name'];

  // Add some additional allowances.
  // TODO: we need to allow site admins to set these via the web form
  // by implementing the _webform_edit_component. This is a quick fix.
  if (in_array('fastq', $input['extensions'])) {
    $input['extensions'][] = 'fq';
  }
  if (in_array('fasta', $input['extensions'])) {
    $input['extensions'][] = 'fa';
    $input['extensions'][] = 'fna';
    $input['extensions'][] = 'faa';
  }

  $found_upstream_input = FALSE;
  foreach ($workflow['steps'] as $step_id => $step_details) {
    if (array_key_exists('input_steps', $step_details) and $step_details['input_steps']) {
      // If this tool is the input for another step.
      if (array_key_exists($full_name, $step_details['input_steps'])) {
        $source_step = $step_details['input_steps'][$full_name]['source_step'];
        if (array_key_exists($source_step, $data_inputs)) {
          $source_cid = $data_inputs[$source_step];
          $found_upstream_input = TRUE;
          for ($j = 0; $j < count($webform->components); $j++) {
            if ($webform->components[$j]['cid'] == $source_cid) {
              $webform->components[$j]['extra']['filtering']['types'] = $input['extensions'];
              $extra['filtering']['addextensions'] = implode(", ", $input['extensions']);
            }
          }
        }
      }
    }
  }
  return $found_upstream_input;
}

/**
 * Retrieves the value set in the workflow for a given input.
 *
 * The workflow organizes values in a nested array based on the heirarchy of
 * the tool's inputs and their conditionals. Therefore, this funtion is
 * recursive in order to descend through the nested array to find the value.
 *
 * @param array $workflow
 *   The Galaxy workflow array.
 * @param int $step_index
 *   The step in the workflow for the tool that the input belongs to.
 * @param string $input_full_name
 *   The full name of the tool input. This name is constructed as the workflow
 *   is recursively parsed. The name represents the heirarchal position of
 *   the tool input which each level separated by a '|' character.
 * @param array $values
 *   Do not use when calling this function. It is only used during recursion.
 * @param int $depth
 *   Do not use when calling this function. It is only used during recursion.
 *
 * @return array
 *   The value of the tool input as set in the workflow configuration.
 */
function tripal_galaxy_build_webform_get_input_value(array $workflow, int $step_index, string $input_full_name, array $values = [], int $depth = 0) {

  // At the first level initialize our values array to the tool inputs.
  if ($depth == 0) {
    $values = $workflow['steps'][$step_index]['tool_inputs'];
  }

  // Pop the first name off the list.
  $names = explode('|', $input_full_name);
  $name = array_shift($names);

  // Check if the $values[$name] is JSON and if so then decode it.
  if (is_array($values) and array_key_exists($name, $values)) {
    if (!is_array($values[$name])) {
      $dvalues = json_decode($values[$name], TRUE);
      if (json_last_error() === JSON_ERROR_NONE) {
        $values[$name] = $dvalues;
      }
    }
  }
  else {
    return '';
  }

  // If we have more names left in the heirarchy then recurse to the next level.
  if (count($names) > 0) {
    $next_name = implode("|", $names);
    return tripal_galaxy_build_webform_get_input_value($workflow, $step_index, $next_name, $values[$name], ++$depth);
  }
  else {
    // We should have a single key/value pair... not an array. If we have
    // an array that is probably because this fucntion is being called
    // by a conditional that has nested cases, so just return an empty
    // string in that case.
    if (!is_array($values[$name])) {
      return $values[$name];
    }
    return '';
  }
}

/**
 * Unpack a default array from the steps of a given workflow.
 */
function tripal_galaxy_build_webform_get_step_defaults($defaults, $tool_input, &$webform_value) {

  // Iterate through all of the defaults for this step.
  foreach ($defaults as $key => $val) {

    // If the value is an object then we want to recurse.
    if (is_object($val)) {
      tripal_galaxy_build_webform_get_step_defaults($val, $tool_input, $webform_value);
    }

    // If thie default is not for this input then skip it.
    if ($tool_input['name'] != $key) {
      continue;
    }

    // Depending on the input type, then the webform element type will
    // be different.
    switch ($tool_input['type']) {
      case 'integer':
        $webform_value = (int) $val;
        break;

      case 'float':
        $webform_value = (float) $val;
        break;

      case 'text':
        $webform_value = $val;
        break;

      case 'select':
        $webform_value = $val;
        break;

      case 'data':
        break;

      case 'boolean':
        if ($val == 'true') {
          $webform_value = 'yes';
        }
        else {
          $webform_value = 'no';
        }
        break;

      case 'data_collection':
        break;

      case 'repeat':
        break;

      default:
        break;
    }
  }
}

/**
 * Invoke a workflow provided by a webform submission.
 */
function tripal_galaxy_invoke_webform_submission($sid, $job = NULL) {

  // Get the workflow details that need execution.
  $submission = tripal_galaxy_get_submission($sid);
  if (!$submission) {
    throw new Exception('The provided submission ID does not exist');
  }

  // Get a connection to the remote galaxy instance.
  $galaxy = tripal_galaxy_get_connection($submission->galaxy_id);
  if (!$galaxy) {
    $error = $galaxy->getError();
    throw new Exception('Cannot connect to Galaxy server.  Galaxy Server response: ' . $error['message']);
  }

  // The array where the workflow input parameters are stored.
  $parameters = [];

  $query = db_select('webform_submitted_data', 'wsd');
  $query->fields('wsd');
  $query->join('webform_component', 'wc', 'wc.cid = wsd.cid');
  $query->join('tripal_galaxy_webform', 'tgw', 'tgw.wf_sid = wsd.sid');
  $query->fields('wc', ['form_key', 'type', 'extra']);
  $query->condition('tgw.tg_sid', $submission->sid);
  $query->condition('wc.nid', $submission->nid);
  $query->orderBy('wsd.cid');
  $submitted_data = $query->execute()->fetchAll();

  // Get the history that we'll use for this submission.
  $error = [];
  $history_name = tripal_galaxy_get_history_name($submission);
  $history = tripal_galaxy_create_history($galaxy, $history_name);
  if (!$history) {
    $error = $galaxy->getError();
    throw new Exception($error['message']);
  }

  // Get the history contents so we don't upload the same file more than once
  // in the event that this invocation occurs more than once.
  $ghistory_contents = new GalaxyHistoryContents($galaxy);
  $history_contents = $ghistory_contents->index([
    'history_id' => $history['id'],
  ]);
  if ($error['message']) {
    $error = $galaxy->getError();
    throw new Exception($error['message']);
  }

  // Iterate through our submitted data to look for files that need uploading.
  // For those in a dataset collection, create those collections as well.
  $collections = [];
  $inputs = [];
  foreach ($submitted_data as $data) {

    // Webform select component doesn't like an option with an empty value
    // so we had to force one by using the special string '--empty--'. Before
    // we invoke the workflow we must switch it back to an empty value.
    if ($data->data == '--empty--') {
      $data->data = '';
    }

    // Handle a single file upload.
    if ($data->type == 'galaxy_sfile') {
      if ($data->no == 'data_collection' or $data->no == 'existing' or preg_match('/^submitted_/', $data->no)) {
        $fid = $data->data;
        if ($fid) {
          $file = tripal_galaxy_upload_file($galaxy, $fid, $history['id'], $history_contents);
          $inputs[$data->form_key] = $file;
        }
      }
      else {
        if ($data->no == 'site_wide') {
          $fid = $data->data;
          if ($fid) {
            $swfile = db_select('tripal_galaxy_site_files', 'tgsf')->fields('tgsf')
              ->condition('fid', $fid)
              ->execute()
              ->fetchObject();
            if (!$swfile) {
              throw new Exception('Cannot find site-wide file with fid: ' . $fid);
            }
            // If this is a remote file....
            if ($swfile->url) {
            }
            // Not a remote file.
            else {
              $file = tripal_galaxy_upload_file($galaxy, $fid, $history['id'], $history_contents);
              $inputs[$data->form_key] = $file;
            }
          }
        }
      }
    }
    else {
      if ($data->type == 'galaxy_sflist') {

        if ($data->no == 'existing' or preg_match('/^submitted_/', $data->no)) {
          $fids = $data->data;
          if (!$fids) {
            continue;
          }
          $set = explode('|', $fids);

          // Create the paired dataset.
          $collections[$data->form_key] = [
            'collection_type' => 'list',
            'instance_type' => 'history',
            'name' => $data->form_key,
            'history_id' => $history['id'],
            'element_identifiers' => [],
          ];

          for ($i = 0; $i < count($set); $i++) {
            // Upload each file.
            if ($set[$i]) {
              $dataset1 = tripal_galaxy_upload_file($galaxy, $set[$i], $history['id'], $history_contents);

              // Next add the element.
              $collections[$data->form_key]['element_identifiers'][] = [
                'id' => $dataset1['id'],
                'name' => $dataset1['name'],
                'src' => 'hda',
              ];
            }
          } // end for ($i = 0; $i < count($set1); $i++) {
        }
      }
      else {
        if ($data->type == 'galaxy_pflist') {
          if ($data->no == 'existing' or preg_match('/^submitted_/', $data->no)) {
            $fids = $data->data;
            if (!$fids) {
              continue;
            }
            $set1 = [];
            $set2 = [];
            if ($fids) {
              list ($set1, $set2) = explode(',', $fids);
              $set1 = explode('|', $set1);
              $set2 = explode('|', $set2);
            }

            // Create the paired dataset.
            $collections[$data->form_key] = [
              'collection_type' => 'list:paired',
              'instance_type' => 'history',
              'name' => $data->form_key,
              'history_id' => $history['id'],
              'element_identifiers' => [],
            ];

            for ($i = 0; $i < count($set1); $i++) {
              // Upload each file.
              if ($set1[$i] and $set2[$i]) {
                $dataset1 = tripal_galaxy_upload_file($galaxy, $set1[$i], $history['id'], $history_contents);
                $dataset2 = tripal_galaxy_upload_file($galaxy, $set2[$i], $history['id'], $history_contents);

                // Next create the paired dataset collection array.
                $collections[$data->form_key]['element_identifiers'][] = [
                  'name' => $dataset1['name'] . ' and ' . $dataset2['name'],
                  'collection_type' => 'paired',
                  'src' => 'new_collection',
                  'element_identifiers' => [
                    [
                      'id' => $dataset1['id'],
                      'name' => 'forward',
                      'src' => 'hda',
                    ],
                    [
                      'id' => $dataset2['id'],
                      'name' => 'reverse',
                      'src' => 'hda',
                    ],
                  ],
                ];
              }
            } // End for ($i = 0; $i < count($set1); $i++) {
          } // end if ($data->no == 'existing' or preg_match('/^submitted_/',
          // $data->no)) {.
        }
      }
    } // end else if($data->type == 'galaxy_pflist') {
  } // foreach ($submitted_data as $data) {

  // Now create a dataset collections.
  foreach ($collections as $form_key => $ds_details) {

    // Check for the dataset in the history before trying to readd it.
    $found_collection = FALSE;
    foreach ($history_contents as $hcontent) {
      if ($hcontent['name'] == $form_key and $hcontent['deleted'] == 0) {
        $inputs[$form_key] = $hcontent;
        $found_collection = TRUE;
      }
    }
    if ($found_collection) {
      $report = "Collection, $form_key, already exists.";
      if (is_object($job)) {
        $job->logMessage($report);
      }
      else {
        print $report . "\n";
      }
      continue;
    }

    // Create the collection!
    $report = "Creating Collection: $form_key";
    if (is_object($job)) {
      $job->logMessage($report);
    }
    else {
      print $report . "\n";
    }
    $gds_collection = new GalaxyDatasetCollections($galaxy);
    $ds_collection = $gds_collection->create($ds_details);
    if (!$ds_collection) {
      $error = $galaxy->getError();
      throw new Exception($error['message']);
    }
    $inputs[$form_key] = $ds_collection;
  }

  // Next we need to re-get the history content to get the IDs of the new files.
  $history_contents = $ghistory_contents->index([
    'history_id' => $history['id'],
  ]);
  $input_datasets = [];

  // Iterate through the submitted data values and create an input aray
  // for Galaxy workflow submission.
  foreach ($submitted_data as $data) {
    $value = '';

    // The component 'extra' data is serialized so we need to convert it.
    $data->extra = unserialize($data->extra);

    // Get some step details.
    $step_index = $data->extra['step_index'];
    $input_name = $data->extra['input_full_name'];

    // Skip components that don't have an 'input_name' field as these
    // don't correspond to workflow input components (e.g. fieldsets, and
    // page separators).
    if (!array_key_exists('input_name', $data->extra)) {
      continue;
    }

    // Input elements of type 'fixed_value' should not be provided as
    // input for the Galaxy workflow because the defaults should be used.
    if ($data->type == 'fixed_value') {
      $value = $data->data;
      if (array_key_exists('linked', $data->extra) and $data->extra['linked']) {
        list ($source_step, $step_output) = explode('|', $data->data);
        $value = [
          'step_output' => $step_output,
          'source_step' => $source_step,
        ];
      }
    }
    else {
      if ($data->type == 'galaxy_sfile') {
        $file = $inputs[$data->form_key];
        foreach ($history_contents as $hcontent) {
          if ($hcontent['name'] == $file['name']) {
            $value = [
              'id' => $hcontent['id'],
              'src' => 'hda',
            ];
            $input_datasets[$step_index] = [
              'id' => $hcontent['id'],
              'src' => 'hda',
            ];
          }
        }
      }
      else {
        if ($data->type == 'galaxy_sflist') {
          $dataset = $inputs[$data->form_key];
          $value = $dataset['id'];
          $input_datasets[$step_index] = [
            'id' => $dataset['id'],
            'src' => 'hdca',
          ];
        }
        else {
          if ($data->type == 'galaxy_pflist') {
            $dataset = $inputs[$data->form_key];
            $value = $dataset['id'];
            $input_datasets[$step_index] = [
              'id' => $dataset['id'],
              'src' => 'hdca',
            ];
          }
          else {
            $value = $data->data;
          }
        }
      }
    }

    // If we don't have a value then skip it and let the workflow use it's
    // own defaults.
    if (!$value) {
      continue;
    }
    if (!array_key_exists($step_index, $parameters)) {
      $parameters[$step_index] = [];
    }
    $current_values = $parameters[$step_index];
    _tripal_galaxy_invoke_webform_build_values($current_values, $input_name, $value);
    $parameters[$step_index] = $current_values;
  }

  // Call the Tripal Galaxy API function to invoke this workflow.
  tripal_galaxy_invoke_workflow($galaxy, $submission, $parameters, $input_datasets, $history);
}

/**
 * A helper function for recursively building a values array.
 *
 * The values array is needed to invoke a Galaxy workflow.
 *
 * @param array $values
 *   The array into which the value will be placed.
 * @param string $input_full_name
 *   The full name of the input paramter.
 * @param scalar $value
 *   The value to add to the array.
 */
function _tripal_galaxy_invoke_webform_build_values(array &$values, string $input_full_name, $value) {
  // Pop the first name off the list.
  $names = explode('|', $input_full_name);
  $name = array_shift($names);

  // As long as we have names in our "name path" then recurse.
  if (count($names) > 0) {
    $temp = [];
    if (array_key_exists($name, $values)) {
      $temp = $values[$name];
    }
    $next_name = implode("|", $names);
    $had_value = _tripal_galaxy_invoke_webform_build_values($temp, $next_name, $value);
    if ($had_value) {
      $values[$name] = $temp;
      return TRUE;
    }
    else {
      return FALSE;
    }
  }
  else {
    if ($value) {
      $values[$name] = $value;
      return TRUE;
    }
    return FALSE;
  }
}
