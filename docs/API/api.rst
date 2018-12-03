Tripal Galaxy API
=================
.. note::

  Use of the Tripal Galaxy API requires the installation of the Tripal Galaxy module and the `blend4php library <https://github.com/galaxyproject/blend4php>`_..

The Tripal Galaxy API allows you to integration Tripal and Galaxy and customize some or all of the process.  By default the Tripal Galaxy module provides an interface that allows a site to offer a step-by-step interface for a workflow, maintaining the look-and-feel of the site.  If that functionality is sufficient you will not need this API documentation.

The blend4php library was specifically built to provide a native PHP interface to the Galaxy API such that any PHP application can connect to the RESTful web services of a remote Galaxy server.  While blend4php provides much of the functions needed, the Tripal Galaxy module does provide a few other functions that help with Tripal integration.  

The API documentation is provided in two forms: an overview and a list of functions. The overview provides a written description for using the API and the list of functions provides the list of all functions, their specific arguments and return values.

.. toctree::
   :maxdepth: 1
   
   ./api_overview
   ./api_funcs