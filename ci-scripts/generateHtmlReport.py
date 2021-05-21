#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# *	the terms found in the LICENSE file in the root of this source tree.
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *	  contact@openairinterface.org
# */
#---------------------------------------------------------------------

import os
import re
import sys
import subprocess

class HtmlReport():
	def __init__(self):
		self.job_name = ''
		self.job_id = ''
		self.job_url = ''
		self.job_start_time = 'TEMPLATE_TIME'
		self.git_url = ''
		self.git_src_branch = ''
		self.git_src_commit = ''
		self.git_src_commit_msg = None
		self.git_pull_request = False
		self.git_target_branch = ''
		self.git_target_commit = ''

	def generate(self):
		cwd = os.getcwd()
		self.file = open(cwd + '/test_results_oai_hss.html', 'w')
		self.generateHeader()

		self.coding_formatting_log()

		self.analyze_sca_log()

		self.buildSummaryHeader()
		self.initialGitSetup()
		self.installLibsPackagesRow()
		self.buildCompileRows()
		self.copyToTargetImage()
		self.copyConfToolsToTargetImage()
		self.imageSizeRow()
		self.buildSummaryFooter()

		self.sanityCheckSummaryHeader()
		self.containerStartRow()
		self.startStopCheckRow()
		self.sanityCheckSummaryFooter()

		self.testSummaryHeader()
		self.testSummaryFooter()

		self.generateFooter()
		self.file.close()

	def generateHeader(self):
		# HTML Header
		self.file.write('<!DOCTYPE html>\n')
		self.file.write('<html class="no-js" lang="en-US">\n')
		self.file.write('<head>\n')
		self.file.write('  <meta name="viewport" content="width=device-width, initial-scale=1">\n')
		self.file.write('  <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css">\n')
		self.file.write('  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>\n')
		self.file.write('  <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js"></script>\n')
		self.file.write('  <title>OAI Core Network Test Results for ' + self.job_name + ' job build #' + self.job_id + '</title>\n')
		self.file.write('</head>\n')
		self.file.write('<body><div class="container">\n')
		self.file.write('  <table width = "100%" style="border-collapse: collapse; border: none;">\n')
		self.file.write('	<tr style="border-collapse: collapse; border: none;">\n')
		self.file.write('	  <td style="border-collapse: collapse; border: none;">\n')
		self.file.write('		<a href="http://www.openairinterface.org/">\n')
		self.file.write('		   <img src="http://www.openairinterface.org/wp-content/uploads/2016/03/cropped-oai_final_logo2.png" alt="" border="none" height=50 width=150>\n')
		self.file.write('		   </img>\n')
		self.file.write('		</a>\n')
		self.file.write('	  </td>\n')
		self.file.write('	  <td style="border-collapse: collapse; border: none; vertical-align: center;">\n')
		self.file.write('		<b><font size = "6">Job Summary -- Job: ' + self.job_name + ' -- Build-ID: <a href="' + self.job_url + '">' + self.job_id + '</a></font></b>\n')
		self.file.write('	  </td>\n')
		self.file.write('	</tr>\n')
		self.file.write('  </table>\n')
		self.file.write('  <br>\n')

		# Build Info Summary
		buildSummary = ''
		buildSummary += '  <table class="table-bordered" width = "80%" align = "center" border = "1">\n'
		buildSummary += '	 <tr>\n'
		buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-time"></span> Build Start Time</td>\n'
		#date_formatted = re.sub('\..*', '', self.created)
		buildSummary += '	   <td>' + self.job_start_time + '</td>\n'
		buildSummary += '	 </tr>\n'
		buildSummary += '	 <tr>\n'
		buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-wrench"></span> Build Trigger</td>\n'
		if self.git_pull_request:
			buildSummary += '	   <td>Pull Request</td>\n'
		else:
			buildSummary += '	   <td>Push Event</td>\n'
		buildSummary += '	 </tr>\n'
		buildSummary += '	 <tr>\n'
		buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-cloud-upload"></span> GIT Repository</td>\n'
		buildSummary += '	   <td><a href="' + self.git_url + '">' + self.git_url + '</a></td>\n'
		buildSummary += '	 </tr>\n'
		if self.git_pull_request:
			buildSummary += '	 <tr>\n'
			buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-link"></span> Pull Request Link</td>\n'
			buildSummary += '	   <td><a href="TEMPLATE_PULL_REQUEST_LINK">TEMPLATE_PULL_REQUEST_LINK</a></td>\n'
			buildSummary += '	 </tr>\n'
			buildSummary += '	 <tr>\n'
			buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-header"></span> Pull Request Title</td>\n'
			buildSummary += '	   <td>TEMPLATE_PULL_REQUEST_TEMPLATE</td>\n'
			buildSummary += '	 </tr>\n'
			buildSummary += '	 <tr>\n'
			buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-log-out"></span> Source Branch</td>\n'
			buildSummary += '	   <td>' + self.git_src_branch + '</td>\n'
			buildSummary += '	 </tr>\n'
			buildSummary += '	 <tr>\n'
			buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-tag"></span> Source Commit ID</td>\n'
			buildSummary += '	   <td>' + self.git_src_commit + '</td>\n'
			buildSummary += '	 </tr>\n'
			if (self.git_src_commit_msg is not None):
				buildSummary += '	 <tr>\n'
				buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-comment"></span> Source Commit Message</td>\n'
				buildSummary += '	   <td>' + self.git_src_commit_msg + '</td>\n'
				buildSummary += '	 </tr>\n'
			buildSummary += '	 <tr>\n'
			buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-log-in"></span> Target Branch</td>\n'
			buildSummary += '	   <td>' + self.git_target_branch + '</td>\n'
			buildSummary += '	 </tr>\n'
			buildSummary += '	 <tr>\n'
			buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-tag"></span> Target Commit ID</td>\n'
			buildSummary += '	   <td>' + self.git_target_commit + '</td>\n'
			buildSummary += '	 </tr>\n'
		else:
			buildSummary += '	 <tr>\n'
			buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-tree-deciduous"></span> Branch</td>\n'
			buildSummary += '	   <td>' + self.git_src_branch + '</td>\n'
			buildSummary += '	 </tr>\n'
			buildSummary += '	 <tr>\n'
			buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-tag"></span> Commit ID</td>\n'
			buildSummary += '	   <td>' + self.git_src_commit + '</td>\n'
			buildSummary += '	 </tr>\n'
			if (self.git_src_commit_msg is not None):
				buildSummary += '	 <tr>\n'
				buildSummary += '	   <td bgcolor="lightcyan" > <span class="glyphicon glyphicon-comment"></span> Commit Message</td>\n'
				buildSummary += '	   <td>' + self.git_src_commit_msg + '</td>\n'
				buildSummary += '	 </tr>\n'
		buildSummary += '  </table>\n'
		buildSummary += '  <br>\n'
		self.file.write(buildSummary)

		cwd = os.getcwd()
		test_reports = ['test_results_oai_epc', 'test_results_magma_epc_rhel8']
		for test in test_reports:
			if os.path.isfile(cwd + '/' + test + '.html'):
				newEpcReport = open(cwd + '/' + test + '_new.html', 'w')
				buildSummaryDone = True
				with open(cwd + '/' + test + '.html', 'r') as originalEpcReport:
					for line in originalEpcReport:
						result = re.search('Deployment Summary', line)
						if (result is not None) and buildSummaryDone:
							newEpcReport.write(buildSummary)
							buildSummaryDone = False
						newEpcReport.write(line)
					originalEpcReport.close()
				newEpcReport.close()
				os.rename(cwd + '/' + test + '_new.html', cwd + '/' + test + '.html')

	def generateFooter(self):
		self.file.write('  <div class="well well-lg">End of Build Report -- Copyright <span class="glyphicon glyphicon-copyright-mark"></span> 2020 <a href="http://www.openairinterface.org/">OpenAirInterface</a>. All Rights Reserved.</div>\n')
		self.file.write('</div></body>\n')
		self.file.write('</html>\n')

	def coding_formatting_log(self):
		cwd = os.getcwd()
		self.file.write('  <h2>OAI Coding / Formatting Guidelines Check</h2>\n')
		if os.path.isfile(cwd + '/src/oai_rules_result.txt'):
			cmd = 'grep NB_FILES_FAILING_CHECK ' + cwd + '/src/oai_rules_result.txt | sed -e "s#NB_FILES_FAILING_CHECK=##"'
			nb_fail = subprocess.check_output(cmd, shell=True, universal_newlines=True)
			cmd = 'grep NB_FILES_CHECKED ' + cwd + '/src/oai_rules_result.txt | sed -e "s#NB_FILES_CHECKED=##"'
			nb_total = subprocess.check_output(cmd, shell=True, universal_newlines=True)
			if int(nb_fail.strip()) == 0:
				self.file.write('  <div class="alert alert-success">\n')
				if self.git_pull_request:
					self.file.write('    <strong>All modified files in Pull-Request follow OAI rules. <span class="glyphicon glyphicon-ok-circle"></span> -> (' + nb_total.strip() + ' were checked)</strong>\n')
				else:
					self.file.write('    <strong>All files in repository follow OAI rules. <span class="glyphicon glyphicon-ok-circle"></span> -> (' + nb_total.strip() + ' were checked)</strong>\n')
				self.file.write('  </div>\n')
			else:
				self.file.write('  <div class="alert alert-warning">\n')
				if self.git_pull_request:
					self.file.write('    <strong>' + nb_fail.strip() + ' modified files in Pull-Request DO NOT follow OAI rules. <span class="glyphicon glyphicon-warning-sign"></span> -> (' + nb_total.strip() + ' were checked)</strong>\n')
				else:
					self.file.write('    <strong>' + nb_fail.strip() + ' files in repository DO NOT follow OAI rules. <span class="glyphicon glyphicon-warning-sign"></span> -> (' + nb_total.strip() + ' were checked)</strong>\n')
				self.file.write('  </div>\n')

				if os.path.isfile(cwd + '/src/oai_rules_result_list.txt'):
					self.file.write('  <button data-toggle="collapse" data-target="#oai-formatting-details">More details on formatting check</button>\n')
					self.file.write('  <div id="oai-formatting-details" class="collapse">\n')
					self.file.write('  <p>Please apply the following command to this(ese) file(s): </p>\n')
					self.file.write('  <p style="margin-left: 30px"><strong><code>cd src && clang-format -i filename(s)</code></strong></p>\n')
					self.file.write('  <table class="table-bordered" width = "60%" align = "center" border = 1>\n')
					self.file.write('    <tr><th bgcolor = "lightcyan" >Filename</th></tr>\n')
					with open(cwd + '/src/oai_rules_result_list.txt', 'r') as filelist:
						for line in filelist:
							self.file.write('    <tr><td>' + line.strip() + '</td></tr>\n')
						filelist.close()
					self.file.write('  </table>\n')
					self.file.write('  </div>\n')
		else:
			self.file.write('  <div class="alert alert-danger">\n')
			self.file.write('	  <strong>Was NOT performed (with CLANG-FORMAT tool). <span class="glyphicon glyphicon-ban-circle"></span></strong>\n')
			self.file.write('  </div>\n')

		self.file.write('  <br>\n')

	def analyze_sca_log(self):
		cwd = os.getcwd()
		if os.path.isfile(cwd + '/archives/cppcheck_build.log'):
			self.file.write('  <h2>Static Code Analysis</h2>\n')
		if os.path.isfile(cwd + '/archives/cppcheck.xml'):
			nb_errors = 0
			nb_warnings = 0
			nb_uninitvar = 0
			nb_uninitStructMember = 0
			nb_memleak = 0
			nb_doubleFree = 0
			nb_resourceLeak = 0
			nb_nullPointer = 0
			nb_arrayIndexOutOfBounds = 0
			nb_bufferAccessOutOfBounds = 0
			nb_unknownEvaluationOrder = 0
			with open(cwd + '/archives/cppcheck.xml', 'r') as xmlfile:
				for line in xmlfile:
					result = re.search('severity="warning"', line)
					if result is not None:
						nb_warnings += 1
					result = re.search('severity="error"', line)
					if result is not None:
						nb_errors += 1
						result = re.search('uninitvar', line)
						if result is not None:
							nb_uninitvar += 1
						result = re.search('uninitStructMember', line)
						if result is not None:
							nb_uninitStructMember += 1
						result = re.search('memleak', line)
						if result is not None:
							nb_memleak += 1
						result = re.search('doubleFree', line)
						if result is not None:
							nb_doubleFree += 1
						result = re.search('resourceLeak', line)
						if result is not None:
							nb_resourceLeak += 1
						result = re.search('nullPointer', line)
						if result is not None:
							nb_nullPointer += 1
						result = re.search('arrayIndexOutOfBounds', line)
						if result is not None:
							nb_arrayIndexOutOfBounds += 1
						result = re.search('bufferAccessOutOfBounds', line)
						if result is not None:
							nb_bufferAccessOutOfBounds += 1
						result = re.search('unknownEvaluationOrder', line)
						if result is not None:
							nb_unknownEvaluationOrder += 1
				xmlfile.close()
			if (nb_errors == 0) and (nb_warnings == 0):
				self.file.write('   <div class="alert alert-success">\n')
				self.file.write('	  <strong>CPPCHECK found NO error and NO warning <span class="glyphicon glyphicon-ok-circle"></span></strong>\n')
				self.file.write('   </div>\n')
			elif (nb_errors == 0):
				self.file.write('   <div class="alert alert-warning">\n')
				self.file.write('	  <strong>CPPCHECK found NO error and ' + str(nb_warnings) + ' warnings <span class="glyphicon glyphicon-warning-sign"></span></strong>\n')
				self.file.write('   </div>\n')
			else:
				self.file.write('   <div class="alert alert-danger">\n')
				self.file.write('	  <strong>CPPCHECK found ' +  str(nb_errors) + ' errors and ' + str(nb_warnings) + ' warnings <span class="glyphicon glyphicon-ban-circle"></span></strong>\n')
				self.file.write('   </div>\n')
			if (nb_errors > 0) or (nb_warnings > 0):
				self.file.write('   <button data-toggle="collapse" data-target="#oai-cppcheck-details">More details on CPPCHECK results</button>\n')
				self.file.write('   <div id="oai-cppcheck-details" class="collapse">\n')
				self.file.write('   <br>\n')
				self.file.write('   <table class="table-bordered" width = "80%" align = "center" border = "1">\n')
				self.file.write('	  <tr bgcolor = "#33CCFF" >\n')
				self.file.write('		<th>Error / Warning Type</th>\n')
				self.file.write('		<th>Nb Errors</th>\n')
				self.file.write('		<th>Nb Warnings</th>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr>\n')
				self.file.write('		<td>Uninitialized variable</td>\n')
				self.file.write('		<td>' + str(nb_uninitvar) + '</td>\n')
				self.file.write('		<td>N/A</td>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr>\n')
				self.file.write('		<td>Uninitialized struct member</td>\n')
				self.file.write('		<td>' + str(nb_uninitStructMember) + '</td>\n')
				self.file.write('		<td>N/A</td>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr>\n')
				self.file.write('		<td>Memory leak</td>\n')
				self.file.write('		<td>' + str(nb_memleak) + '</td>\n')
				self.file.write('		<td>N/A</td>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr>\n')
				self.file.write('		<td>Memory is freed twice</td>\n')
				self.file.write('		<td>' + str(nb_doubleFree) + '</td>\n')
				self.file.write('		<td>N/A</td>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr>\n')
				self.file.write('		<td>Resource leak</td>\n')
				self.file.write('		<td>' + str(nb_resourceLeak) + '</td>\n')
				self.file.write('		<td>N/A</td>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr>\n')
				self.file.write('		<td>Possible null pointer dereference</td>\n')
				self.file.write('		<td>' + str(nb_nullPointer) + '</td>\n')
				self.file.write('		<td>N/A</td>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr>\n')
				self.file.write('		<td>Array access  out of bounds</td>\n')
				self.file.write('		<td>' + str(nb_arrayIndexOutOfBounds) + '</td>\n')
				self.file.write('		<td>N/A</td>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr>\n')
				self.file.write('		<td>Buffer is accessed out of bounds</td>\n')
				self.file.write('		<td>' + str(nb_bufferAccessOutOfBounds) + '</td>\n')
				self.file.write('		<td>N/A</td>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr>\n')
				self.file.write('		<td>Expression depends on order of evaluation of side effects</td>\n')
				self.file.write('		<td>' + str(nb_unknownEvaluationOrder) + '</td>\n')
				self.file.write('		<td>N/A</td>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr>\n')
				self.file.write('		<td>Others</td>\n')
				nb_others = nb_uninitvar + nb_uninitStructMember + nb_memleak + nb_doubleFree + nb_resourceLeak + nb_nullPointer + nb_arrayIndexOutOfBounds + nb_arrayIndexOutOfBounds + nb_bufferAccessOutOfBounds + nb_unknownEvaluationOrder
				nb_others = nb_errors - nb_others
				self.file.write('		<td>' + str(nb_others) + '</td>\n')
				self.file.write('		<td>' + str(nb_warnings) + '</td>\n')
				self.file.write('	  </tr>\n')
				self.file.write('	  <tr bgcolor = "#33CCFF" >\n')
				self.file.write('		<th>Total</th>\n')
				self.file.write('		<th>' + str(nb_errors) + '</th>\n')
				self.file.write('		<th>' + str(nb_warnings) + '</th>\n')
				self.file.write('	  </tr>\n')
				self.file.write('   </table>\n')
				self.file.write('   <br>\n')
				self.file.write('   <p>Full details in artifact (cppcheck.xml) </p>\n')
				self.file.write('   <p style="margin-left: 30px">Graphical Interface tool : <strong><code>cppcheck-gui -l cppcheck.xml</code></strong></p>\n')
				self.file.write('   <br>\n')
				self.file.write('   </div>\n')
		else:
			self.file.write('  <div class="alert alert-danger">\n')
			self.file.write('	  <strong>Was NOT performed (with CPPCHECK tool). <span class="glyphicon glyphicon-ban-circle"></span></strong>\n')
			self.file.write('  </div>\n')

	def buildSummaryHeader(self):
		self.file.write('  <h2>Docker / Podman Image Build Summary</h2>\n')
		self.file.write('  <table class="table-bordered" width = "100%" align = "center" border = "1">\n')
		self.file.write('	  <tr bgcolor="#33CCFF" >\n')
		self.file.write('		<th>Stage Name</th>\n')
		self.file.write('		<th>Image Kind</th>\n')
		self.file.write('		<th>OAI HSS <font color="Gold">Ubuntu18</font> Image</th>\n')
		self.file.write('		<th>OAI HSS <font color="Gold">RHEL8</font> Image</th>\n')
		self.file.write('	  </tr>\n')

	def buildSummaryFooter(self):
		self.file.write('  </table>\n')
		self.file.write('  <br>\n')

	def initialGitSetup(self):
		self.file.write('	 <tr>\n')
		self.file.write('	   <td bgcolor="lightcyan" >Initial Git Setup</td>\n')
		self.analyze_docker_build_git_part('HSS')
		self.file.write('	 </tr>\n')

	def analyze_docker_build_git_part(self, nfType):
		if nfType != 'HSS':
			self.file.write('      <td>N/A</td>\n')
			self.file.write('	   <td colspan="2">Wrong NF Type for this Report</td>\n')
			return

		self.file.write('      <td>Builder Image</td>\n')
		cwd = os.getcwd()

		variants = ['docker', 'podman']
		for variant in variants:
			logFileName = 'hss_' + variant + '_image_build.log'
			if os.path.isfile(cwd + '/archives/' + logFileName):
				status = False
				section_start_pattern = 'git config --global http'
				section_end_pattern = 'WORKDIR /openair-hss/scripts'
				section_status = False
				with open(cwd + '/archives/' + logFileName, 'r') as logfile:
					for line in logfile:
						result = re.search(section_start_pattern, line)
						if result is not None:
							section_status = True
						result = re.search(section_end_pattern, line)
						if result is not None:
							section_status = False
							status = True
					logfile.close()
				if status:
					cell_msg = '	  <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
					cell_msg += 'OK:\n'
					cell_msg += ' -- All Git Operations went successfully</b></pre></td>\n'
				else:
					cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
					cell_msg += 'KO::\n'
					cell_msg += ' -- Some Git Operations went WRONG</b></pre></td>\n'
			else:
				cell_msg = '      <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
				cell_msg += 'KO: logfile (' + logFileName + ') not found</b></pre></td>\n'
			self.file.write(cell_msg)

	def installLibsPackagesRow(self):
		self.file.write('	 <tr>\n')
		self.file.write('	   <td bgcolor="lightcyan" >SW libs and packages Installation</td>\n')
		self.analyze_install_log('HSS')
		self.file.write('	 </tr>\n')

	def analyze_install_log(self, nfType):
		if nfType != 'HSS':
			self.file.write('      <td>N/A</td>\n')
			self.file.write('	   <td colspan="2">Wrong NF Type for this Report</td>\n')
			return

		self.file.write('      <td>Builder Image</td>\n')
		cwd = os.getcwd()

		variants = ['docker', 'podman']
		for variant in variants:
			logFileName = 'hss_' + variant + '_image_build.log'
			if os.path.isfile(cwd + '/archives/' + logFileName):
				status = False
				section_start_pattern = 'build_hss_rel14 --check-installed-software --force'
				section_end_pattern = 'build_hss_rel14 --clean --build-type Release'
				section_status = False
				package_install = False
				freeDiameter_build_start = False
				freeDiameter_build = False
				cares_build_start = False
				cares_build = False
				cppdriver_build_start = False
				cppdriver_build = False
				pistache_build_start = False
				pistache_build = False
				with open(cwd + '/archives/' + logFileName, 'r') as logfile:
					for line in logfile:
						result = re.search(section_start_pattern, line)
						if result is not None:
							section_status = True
						result = re.search(section_end_pattern, line)
						if result is not None:
							section_status = False
						if section_status:
							result = re.search('HSS software installation successful', line)
							if result is not None:
								status = True
							result = re.search('git submodule init', line)
							if result is not None:
								package_install = True
							result = re.search('Build files have been written to: /openair-hss/build/git_submodules/freeDiameter/build', line)
							if result is not None:
								freeDiameter_build_start = True
							if freeDiameter_build_start:
								result = re.search('Installing: /usr/local/lib/freeDiameter/dict_S9', line)
								if result is not None:
									freeDiameter_build_start = False
									freeDiameter_build = True
							result = re.search('/openair-hss/build/git_submodules/c-ares /openair-hss', line)
							if result is not None:
								cares_build_start = True
							if cares_build_start:
								result = re.search('/usr/bin/install -c -m 644 libcares.pc', line)
								if result is not None:
									cares_build_start = False
									cares_build = True
							result = re.search('/openair-hss/build/git_submodules/cpp-driver /openair-hss', line)
							if result is not None:
								cppdriver_build_start = True
							if cppdriver_build_start:
								if variant == 'docker':
									result = re.search('Installing: /usr/local/lib/x86_64-linux-gnu/pkgconfig/cassandra.pc', line)
								else:
									result = re.search('Installing: /usr/local/lib64/pkgconfig/cassandra.pc', line)
								if result is not None:
									cppdriver_build_start = False
									cppdriver_build = True
							result = re.search('/openair-hss/build/git_submodules/pistache /openair-hss', line)
							if result is not None:
								pistache_build_start = True
							if pistache_build_start:
								if variant == 'docker':
									result = re.search('Installing: /usr/local/lib/libpistache', line)
								else:
									result = re.search('Installing: /usr/local/lib64/libpistache', line)
								if result is not None:
									pistache_build_start = False
									pistache_build = True
					logfile.close()
				if status:
					cell_msg = '	  <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
					cell_msg += 'OK:\n'
				else:
					cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
					cell_msg += 'KO:\n'
				cell_msg += ' -- build_hss_rel14 --check-installed-software --force\n'
				if package_install:
					cell_msg += '   ** Packages Installation: OK\n'
				else:
					cell_msg += '   ** Packages Installation: KO\n'
				if freeDiameter_build:
					cell_msg += '   ** FreeDiameter Installation: OK\n'
				else:
					cell_msg += '   ** FreeDiameter Installation: KO\n'
				if cares_build:
					cell_msg += '   ** C-Ares Installation: OK\n'
				else:
					cell_msg += '   ** C-Ares Installation: KO\n'
				if cppdriver_build:
					cell_msg += '   ** Cassandra Driver Installation: OK\n'
				else:
					cell_msg += '   ** Cassandra Driver Installation: KO\n'
				if pistache_build:
					cell_msg += '   ** Pistache Installation: OK\n'
				else:
					cell_msg += '   ** Pistache Installation: KO\n'
				cell_msg += '</b></pre></td>\n'
			else:
				cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
				cell_msg += 'KO: logfile (' + logFileName + ') not found</b></pre></td>\n'
			self.file.write(cell_msg)

	def buildCompileRows(self):
		self.file.write('	 <tr>\n')
		self.file.write('	   <td rowspan=2 bgcolor="lightcyan" >cNF Compile / Build</td>\n')
		self.analyze_build_log('HSS')
		self.file.write('	 </tr>\n')
		self.file.write('	 <tr>\n')
		self.analyze_compile_log('HSS')
		self.file.write('	 </tr>\n')

	def analyze_build_log(self, nfType):
		if nfType != 'HSS':
			self.file.write('      <td>N/A</td>\n')
			self.file.write('	   <td colspan="2">Wrong NF Type for this Report</td>\n')
			return

		self.file.write('      <td>Builder Image</td>\n')
		cwd = os.getcwd()

		variants = ['docker', 'podman']
		for variant in variants:
			logFileName = 'hss_' + variant + '_image_build.log'
			if os.path.isfile(cwd + '/archives/' + logFileName):
				status = False
				section_start_pattern = 'build_hss_rel14 --clean --build-type Release'
				section_end_pattern = 'FROM ubuntu:bionic as oai-hss$'
				section_status = False
				with open(cwd + '/archives/' + logFileName, 'r') as logfile:
					for line in logfile:
						result = re.search(section_start_pattern, line)
						if result is not None:
							section_status = True
						result = re.search(section_end_pattern, line)
						if result is not None:
							section_status = False
						if section_status:
							result = re.search('oai_hss installed', line)
							if result is not None:
								status = True
					logfile.close()
				if status:
					cell_msg = '	  <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
					cell_msg += 'OK:\n'
				else:
					cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
					cell_msg += 'KO:\n'
				cell_msg += ' -- build_hss_rel14 --clean --build-type Release</b></pre></td>\n'
			else:
				cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
				cell_msg += 'KO: logfile (' + logFileName + ') not found</b></pre></td>\n'
			self.file.write(cell_msg)

	def analyze_compile_log(self, nfType):
		if nfType != 'HSS':
			self.file.write('      <td>N/A</td>\n')
			self.file.write('	   <td colspan="2">Wrong NF Type for this Report</td>\n')
			return

		self.file.write('      <td>Builder Image</td>\n')
		cwd = os.getcwd()

		variants = ['docker', 'podman']
		for variant in variants:
			logFileName = 'hss_' + variant + '_image_build.log'
			nb_errors = 0
			nb_warnings = 0
			if os.path.isfile(cwd + '/archives/' + logFileName):
				section_start_pattern = 'build_hss_rel14 --clean --build-type Release'
				section_end_pattern = 'FROM ubuntu:bionic as oai-hss$'
				section_status = False
				with open(cwd + '/archives/' + logFileName, 'r') as logfile:
					for line in logfile:
						result = re.search(section_start_pattern, line)
						if result is not None:
							section_status = True
						result = re.search(section_end_pattern, line)
						if result is not None:
							section_status = False
						if section_status:
							result = re.search('error:', line)
							if result is not None:
								nb_errors += 1
							result = re.search('warning:', line)
							if result is not None:
								nb_warnings += 1
					logfile.close()
				if nb_warnings == 0 and nb_errors == 0:
					cell_msg = '	   <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
				elif nb_warnings < 20 and nb_errors == 0:
					cell_msg = '	   <td bgcolor="Orange"><pre style="border:none; background-color:Orange"><b>'
				else:
					cell_msg = '	   <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
				if nb_errors > 0:
					cell_msg += str(nb_errors) + ' errors found in compile log\n'
				cell_msg += str(nb_warnings) + ' warnings found in compile log</b></pre></td>\n'
			else:
				cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
				cell_msg += 'KO: logfile (' + logFileName + ') not found</b></pre></td>\n'
			self.file.write(cell_msg)

	def copyToTargetImage(self):
		self.file.write('	 <tr>\n')
		self.file.write('	   <td bgcolor="lightcyan" >SW libs Installation / Copy from Builder</td>\n')
		self.analyze_copy_log('HSS')
		self.file.write('	 </tr>\n')

	def analyze_copy_log(self, nfType):
		if nfType != 'HSS':
			self.file.write('      <td>N/A</td>\n')
			self.file.write('	   <td colspan="2">Wrong NF Type for this Report</td>\n')
			return

		self.file.write('      <td>Target Image</td>\n')
		cwd = os.getcwd()

		variants = ['docker', 'podman']
		for variant in variants:
			logFileName = 'hss_' + variant + '_image_build.log'
			if os.path.isfile(cwd + '/archives/' + logFileName):
				section_start_pattern = 'FROM ubuntu:bionic as oai-hss$'
				section_end_pattern = 'WORKDIR /openair-hss/etc'
				section_status = False
				status = False
				with open(cwd + '/archives/' + logFileName, 'r') as logfile:
					for line in logfile:
						result = re.search(section_start_pattern, line)
						if result is not None:
							section_status = True
						result = re.search(section_end_pattern, line)
						if result is not None:
							section_status = False
							status = True
					logfile.close()
				if status:
					cell_msg = '	   <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
					cell_msg += 'OK:\n'
				else:
					cell_msg = '	   <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
					cell_msg += 'KO:\n'
				cell_msg += '</b></pre></td>\n'
			else:
				cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
				cell_msg += 'KO: logfile (' + logFileName + ') not found</b></pre></td>\n'
			self.file.write(cell_msg)

	def copyConfToolsToTargetImage(self):
		self.file.write('	 <tr>\n')
		self.file.write('	   <td bgcolor="lightcyan" >Copy Template Conf / Tools from Builder</td>\n')
		self.analyze_copy_conf_tool_log('HSS')
		self.file.write('	 </tr>\n')

	def analyze_copy_conf_tool_log(self, nfType):
		if nfType != 'HSS':
			self.file.write('      <td>N/A</td>\n')
			self.file.write('	   <td colspan="2">Wrong NF Type for this Report</td>\n')
			return

		self.file.write('      <td>Target Image</td>\n')
		cwd = os.getcwd()

		variants = ['docker', 'podman']
		for variant in variants:
			logFileName = 'hss_'+ variant + '_image_build.log'
			if os.path.isfile(cwd + '/archives/' + logFileName):
				section_start_pattern = 'WORKDIR /openair-hss/etc'
				if variant == 'docker':
					section_end_pattern = 'Successfully tagged oai-hss'
				else:
					section_end_pattern = 'COMMIT oai-hss:'
				section_status = False
				status = False
				with open(cwd + '/archives/' + logFileName, 'r') as logfile:
					for line in logfile:
						result = re.search(section_start_pattern, line)
						if result is not None:
							section_status = True
						result = re.search(section_end_pattern, line)
						if result is not None:
							section_status = False
							status = True
					logfile.close()
				if status:
					cell_msg = '	   <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
					cell_msg += 'OK:\n'
				else:
					cell_msg = '	   <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
					cell_msg += 'KO:\n'
				cell_msg += '</b></pre></td>\n'
			else:
				cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
				cell_msg += 'KO: logfile (' + logFileName + ') not found</b></pre></td>\n'
			self.file.write(cell_msg)

	def imageSizeRow(self):
		self.file.write('	 <tr>\n')
		self.file.write('	   <td bgcolor="lightcyan" >Image Size</td>\n')
		self.analyze_image_size_log('HSS')
		self.file.write('	 </tr>\n')

	def analyze_image_size_log(self, nfType):
		if nfType != 'HSS':
			self.file.write('      <td>N/A</td>\n')
			self.file.write('	   <td colspan="2">Wrong NF Type for this Report</td>\n')
			return

		self.file.write('      <td>Target Image</td>\n')
		cwd = os.getcwd()

		variants = ['docker', 'podman']
		for variant in variants:
			logFileName = 'hss_' + variant + '_image_build.log'
			if os.path.isfile(cwd + '/archives/' + logFileName):
				section_start_pattern = 'Successfully tagged oai-hss'
				if variant == 'docker':
					section_start_pattern = 'Successfully tagged oai-hss'
				else:
					section_start_pattern = 'COMMIT oai-hss:'
				section_end_pattern = 'OAI-HSS DOCKER IMAGE BUILD'
				section_status = False
				status = False
				with open(cwd + '/archives/' + logFileName, 'r') as logfile:
					for line in logfile:
						result = re.search(section_start_pattern, line)
						if result is not None:
							section_status = True
						result = re.search(section_end_pattern, line)
						if result is not None:
							section_status = False
						if section_status:
							if self.git_pull_request:
								result = re.search('oai-hss *ci-temp', line)
							else:
								result = re.search('oai-hss *develop', line)
							if result is not None:
								result = re.search('ago *([0-9 A-Z]+)', line)
								if result is not None:
									size = result.group(1)
									status = True
					logfile.close()
				if status:
					cell_msg = '	   <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
					cell_msg += 'OK:  ' + size + '\n'
				else:
					cell_msg = '	   <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
					cell_msg += 'KO:\n'
				cell_msg += '</b></pre></td>\n'
			else:
				cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
				cell_msg += 'KO: logfile (' + logFileName + ') not found</b></pre></td>\n'
			self.file.write(cell_msg)

	def sanityCheckSummaryHeader(self):
		self.file.write('  <h2>Ubuntu18 Sanity Check Deployment Summary</h2>\n')
		self.file.write('  <table class="table-bordered" width = "100%" align = "center" border = "1">\n')
		self.file.write('	  <tr bgcolor="#33CCFF" >\n')
		self.file.write('		<th>Stage Name</th>\n')
		self.file.write('		<th>OAI MME cNF</th>\n')
		self.file.write('		<th>OAI HSS cNF</th>\n')
		self.file.write('		<th>Cassandra</th>\n')
		self.file.write('	  </tr>\n')

	def sanityCheckSummaryFooter(self):
		self.file.write('  </table>\n')
		self.file.write('  <br>\n')

	def containerStartRow(self):
		self.file.write('	 <tr>\n')
		self.file.write('	   <td bgcolor="lightcyan" >Starting/Configuring Docker Containers</td>\n')
		self.analyze_docker_start_log('MME')
		self.analyze_docker_start_log('HSS')
		self.analyze_docker_start_log('Cassandra')
		self.file.write('	 </tr>\n')

	def analyze_docker_start_log(self, nfType):
		if nfType == 'Cassandra':
			logFileName = 'cassandra_status.log'
			pattern = 'CASSANDRA START:'
		else:
			logFileName = nfType.lower() + '_config.log'
			pattern = 'OAI-' + nfType.upper() + ' CONFIG:'

		cwd = os.getcwd()
		if os.path.isfile(cwd + '/archives/' + logFileName):
			status = False
			racCassandraMsg = False
			nb_subs = 0
			nb_mme_isdn = 0
			nb_certificate = 0
			with open(cwd + '/archives/' + logFileName, 'r') as logfile:
				for line in logfile:
					result = re.search(pattern, line)
					if result is not None:
						result = re.search('OK', line)
						if result is not None:
							status = True
					if nfType == 'Cassandra':
						result = re.search('RAC1', line)
						if result is not None:
							racCassandraMsg = True
					if nfType == "HSS":
						result = re.search('Subscription-Data', line)
						if result is not None:
							nb_subs += 1
						result = re.search('mme-isdn', line)
						if result is not None:
							nb_mme_isdn += 1
						result = re.search('Certificate is to be certified', line)
						if result is not None:
							nb_certificate += 1
					if nfType == "MME":
						result = re.search('Certificate is to be certified', line)
						if result is not None:
							nb_certificate += 1
				logfile.close()
			if nfType == 'Cassandra':
				if status and racCassandraMsg:
					cell_msg = '	  <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
					cell_msg += 'OK: ci-cassandra:\n'
					cell_msg += ' -- started successfully\n'
					cell_msg += ' -- service status is OK</b></pre></td>\n'
				elif not racCassandraMsg:
					cell_msg = '	  <td bgcolor="Orange"><pre style="border:none; background-color:Orange"><b>'
					cell_msg += 'KO: ci-cassandra:\n'
					cell_msg += ' -- started successfully\n'
					cell_msg += ' -- service status is missing?</b></pre></td>\n'
				else:
					cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
					cell_msg += 'KO: ci-cassandra:\n'
					cell_msg += ' -- did not start properly?</b></pre></td>\n'
			else:
				if status:
					cell_msg = '	  <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
					cell_msg += 'OK: ci-oai-' + nfType.lower() + ':\n'
					cell_msg += ' -- started successfully\n'
				else:
					cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
					cell_msg += 'KO: ci-oai-' + nfType.lower() + ':\n'
					cell_msg += ' -- did not start properly?\n'
				if nfType == 'HSS':
					cell_msg += ' -- ' + str(nb_subs) + ' users were provisioned\n'
					cell_msg += ' -- ' + str(int(nb_mme_isdn / 2)) + ' mme instance(s) were provisioned\n'
					if nb_certificate > 0:
						cell_msg += ' -- certificates were generated</b></pre></td>\n'
					else:
						cell_msg += ' -- certificates were NOT generated</b></pre></td>\n'
				if nfType == 'MME':
					if nb_certificate > 0:
						cell_msg += ' -- certificates were generated</b></pre></td>\n'
					else:
						cell_msg += ' -- certificates were NOT generated</b></pre></td>\n'
		else:
			cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
			cell_msg += 'KO: logfile (' + logFileName + ') not found</b></pre></td>\n'

		self.file.write(cell_msg)

	def startStopCheckRow(self):
		self.file.write('	 <tr>\n')
		self.file.write('	   <td bgcolor="lightcyan" >cNF Check Start / Stop</td>\n')
		self.analyze_check_run_log('HSS')
		self.analyze_check_run_log('MME')
		self.analyze_check_run_log('Cassandra')
		self.file.write('	 </tr>\n')

	def analyze_check_run_log(self, nfType):
		if nfType == 'Cassandra':
			cell_msg = '	  <td bgcolor="LightGray"><pre style="border:none; background-color:LightGray"><b>'
			cell_msg += 'N/A</b></pre></td>\n'
			self.file.write(cell_msg)
			return

		logFileName = nfType.lower() + '_check_run.log'

		cwd = os.getcwd()
		if os.path.isfile(cwd + '/archives/' + logFileName):
			myCmd = 'iconv -f ISO-8859-1 -t UTF-8//TRANSLIT ' + cwd + '/archives/' + logFileName + ' -o ' + cwd + '/archives/' + logFileName + '.conv'
			subprocess.run(myCmd, shell=True)
			myCmd = 'mv ' + cwd + '/archives/' + logFileName + '.conv '  + cwd + '/archives/' + logFileName
			subprocess.run(myCmd, shell=True)
			nb_opc_generation = 0
			freeDiameterUp = False
			connectionWithMME = False
			connectionWithHSS = False
			sctp_status = False
			with open(cwd + '/archives/' + logFileName, 'r') as logfile:
				for line in logfile:
					result = re.search('Compute opc', line)
					if result is not None:
						nb_opc_generation += 1
					result = re.search('The freeDiameter engine has been started|Diameter identity of MME', line)
					if result is not None:
						freeDiameterUp = True
					result = re.search('STATE_OPEN.*mme', line)
					if result is not None:
						connectionWithMME = True
					result = re.search('Peer hss.* is now connected', line)
					if result is not None:
						connectionWithHSS = True
					result = re.search('Received SCTP_INIT_MSG', line)
					if result is not None:
						sctp_status = True
				logfile.close()
			if nfType == 'HSS':
				if nb_opc_generation > 0 and freeDiameterUp and connectionWithMME:
					cell_msg = '	  <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
					cell_msg += 'OK:\n'
				else:
					cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
					cell_msg += 'KO:\n'
				cell_msg += '  -- ' + str(nb_opc_generation) + ' OPC were re-generated\n'
				if freeDiameterUp:
					cell_msg += '  -- Free Diameter engine is UP\n'
				else:
					cell_msg += '  -- Free Diameter engine is DOWN?\n'
				if connectionWithMME:
					cell_msg += '  -- Connection w/ MME is OK\n'
				else:
					cell_msg += '  -- Connection w/ MME is KO\n'
				cell_msg += '</b></pre></td>\n'
			if nfType == 'MME':
				if freeDiameterUp and connectionWithHSS and sctp_status:
					cell_msg = '	  <td bgcolor="LimeGreen"><pre style="border:none; background-color:LimeGreen"><b>'
					cell_msg += 'OK:\n'
				else:
					cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
					cell_msg += 'KO:\n'
				if freeDiameterUp:
					cell_msg += '  -- Free Diameter engine is UP\n'
				else:
					cell_msg += '  -- Free Diameter engine is DOWN?\n'
				if connectionWithHSS:
					cell_msg += '  -- Connection w/ HSS is OK\n'
				else:
					cell_msg += '  -- Connection w/ HSS is KO\n'
				if sctp_status:
					cell_msg += '  -- SCTP is OK\n'
				else:
					cell_msg += '  -- SCTP is KO?\n'
				cell_msg += '</b></pre></td>\n'
		else:
			cell_msg = '	  <td bgcolor="Tomato"><pre style="border:none; background-color:Tomato"><b>'
			cell_msg += 'KO: logfile (' + logFileName + ') not found</b></pre></td>\n'

		self.file.write(cell_msg)

	def testSummaryHeader(self):
		self.file.write('  <h2>Test Summary</h2>\n')
		self.file.write('  <div class="alert alert-warning">\n')
		self.file.write('	  <strong>Not performed yet. <span class="glyphicon glyphicon-warning-sign"></span></strong>\n')
		self.file.write('  </div>\n')

	def testSummaryFooter(self):
		self.file.write('  <br>\n')

def Usage():
	print('----------------------------------------------------------------------------------------------------------------------')
	print('generateHtmlReport.py')
	print('   Generate an HTML report for the Jenkins pipeline on openair-hss.')
	print('----------------------------------------------------------------------------------------------------------------------')
	print('Usage: python3 generateHtmlReport.py [options]')
	print('  --help  Show this help.')
	print('---------------------------------------------------------------------------------------------- Mandatory Options -----')
	print('  --job_name=[Jenkins Job name]')
	print('  --job_id=[Jenkins Job Build ID]')
	print('  --job_url=[Jenkins Job Build URL]')
	print('  --git_url=[Git Repository URL]')
	print('  --git_src_branch=[Git Source Branch Name]')
	print('  --git_src_commit=[Git Source Commit SHA-ONE]')
	print('----------------------------------------------------------------------------------------------- Optional Options -----')
	print('  --git_pull_request=True')
	print('  --git_target_branch=[Git Target Branch Name]')
	print('  --git_target_commit=[Git Target Commit SHA-ONE]')

#--------------------------------------------------------------------------------------------------------
#
# Start of main
#
#--------------------------------------------------------------------------------------------------------

argvs = sys.argv
argc = len(argvs)

HTML = HtmlReport()

while len(argvs) > 1:
	myArgv = argvs.pop(1)
	if re.match('^\-\-help$', myArgv, re.IGNORECASE):
		Usage()
		sys.exit(0)
	elif re.match('^\-\-job_name=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-job_name=(.+)$', myArgv, re.IGNORECASE)
		HTML.job_name = matchReg.group(1)
	elif re.match('^\-\-job_id=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-job_id=(.+)$', myArgv, re.IGNORECASE)
		HTML.job_id = matchReg.group(1)
	elif re.match('^\-\-job_url=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-job_url=(.+)$', myArgv, re.IGNORECASE)
		HTML.job_url = matchReg.group(1)
	elif re.match('^\-\-git_url=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-git_url=(.+)$', myArgv, re.IGNORECASE)
		HTML.git_url = matchReg.group(1)
	elif re.match('^\-\-git_src_branch=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-git_src_branch=(.+)$', myArgv, re.IGNORECASE)
		HTML.git_src_branch = matchReg.group(1)
	elif re.match('^\-\-git_src_commit=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-git_src_commit=(.+)$', myArgv, re.IGNORECASE)
		HTML.git_src_commit = matchReg.group(1)
	elif re.match('^\-\-git_src_commit_msg=(.+)$', myArgv, re.IGNORECASE):
		# Not Mandatory
		matchReg = re.match('^\-\-git_src_commit_msg=(.+)$', myArgv, re.IGNORECASE)
		HTML.git_src_commit_msg = matchReg.group(1)
	elif re.match('^\-\-git_pull_request=(.+)$', myArgv, re.IGNORECASE):
		# Can be silent: would be false!
		matchReg = re.match('^\-\-git_pull_request=(.+)$', myArgv, re.IGNORECASE)
		if matchReg.group(1) == 'true' or matchReg.group(1) == 'True':
			HTML.git_pull_request = True
	elif re.match('^\-\-git_target_branch=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-git_target_branch=(.+)$', myArgv, re.IGNORECASE)
		HTML.git_target_branch = matchReg.group(1)
	elif re.match('^\-\-git_target_commit=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-git_target_commit=(.+)$', myArgv, re.IGNORECASE)
		HTML.git_target_commit = matchReg.group(1)
	else:
		sys.exit('Invalid Parameter: ' + myArgv)

if HTML.job_name == '' or HTML.job_id == '' or HTML.job_url == '':
	sys.exit('Missing Parameter in job description')

if HTML.git_url == '' or HTML.git_src_branch == '' or HTML.git_src_commit == '':
	sys.exit('Missing Parameter in Git Repository description')

if HTML.git_pull_request:
	if HTML.git_target_commit == '' or HTML.git_target_branch == '':
		 sys.exit('Missing Parameter in Git Pull Request Repository description')

HTML.generate()
