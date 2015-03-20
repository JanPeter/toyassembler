// Mixing jQuery and Node.js code in the same file? Yes please!

var files = [];
var dirs = [];
var pages = ['folders', 'files', 'settings'];
var page = 'folders';
var extensions = [{name: 'asm'}];

var app = {
	init: false,
	files: {},
	folders: [],
	pages: ['folders', 'files', 'settings'],
	page: 'folders',
	exts: [],
	dest: '.hex',
	drop: null
};

function load()
{

	if(localStorage.dest)
		app.dest = localStorage.dest;

	for(var key in localStorage)
	{
		if(key.indexOf('ext') >= 0)
		{
			app.exts.push({name: localStorage[key]});
		}

		if(key.indexOf('dir') >= 0)
		{
			app.folders.push({path: localStorage[key]});
		}
	}

	if(app.exts.length == 0)
		app.exts = extensions;

}
load();

function save()
{
	localStorage.clear();

	for(var key in app)
	{
		if(key == 'dest')
		{
			localStorage[key] = app[key];
		}
	}

	for(var i=0; i < app.exts.length; i++)
	{
		localStorage['ext'+i] = app.exts[i].name;
	}

	for(var key in app.folders)
	{
		if(app.folders[key].path)
			localStorage['dir'+key] = app.folders[key].path;
	}
}

Array.prototype.ofind = function(fn) {
	for(var k in this)
	{
		if(fn(this[k]))
			return true;
	}
	return false;
}

Array.prototype.find = function(value) {
	for(var k in this)
	{
		if(Array.isArray(this[k]))
			return this[k].find(value);
		else
		{
			return this[k] == value;
		}
	}
	return false;
}

angular.module('toyasm', [])
	.controller('SettingsController', function($scope, $timeout) {
		$scope.settings = {};
		$scope.settings.dest = app.dest;
		$scope.settings.exts = app.exts;
		var saveTimeout = null;

		$scope.add = function() {
			$scope.settings.exts.push({name: ''});
			app.files.search();
		}

		$scope.delete = function(i) {
			$scope.settings.exts.splice(i, 1);
			app.files.search();
		}

		$scope.$watch('settings.dest', function(oldVal, newVal) {
			$scope.save();
		});
		$scope.$watch('settings.exts', function(oldVal, newVal) {
			$scope.save();
			app.files.search();
		}, true);

		$scope.save = function() {
			if(saveTimeout != null)
				$timeout.cancel(saveTimeout);

			saveTimeout = $timeout(function() {
				saveTimeout = null;
				app.exts = $scope.settings.exts;
				app.dest = $scope.settings.dest;
				save();
			}, 500);
		}
	})

	.controller('FilesController', ['$scope', function($scope) {
		$scope.files = [];
		var chok = require('chokidar');
		var fs = require('fs');
		var watcher = [];
		var reload = null;
		var isLoading = false;

		app.files.add = function(file) {
			$scope.files.push(file);
			$scope.$apply();
		}

		app.files.search = function() {
			isLoading = true;
			$scope.files = [];
			for(var key in app.folders)
			{
				if(watcher[key] && watcher[key].close)
					watcher[key].close();
			}
			watcher = [];

			for(var key in app.folders)
			{
				if(app.folders[key].path)
				{
					app.files.recursiveSearch(app.folders[key].path);

					watcher.push(chok.watch(app.folders[key].path).on('all', function(e, p) {

						if(!isLoading)
						{
							var isMyFile = false;
							for(var i in app.exts)
							{
								if(app.exts[i].name)
								{
									if(p.indexOf(app.exts[i].name) == p.length - app.exts[i].name.length)
										isMyFile = true;
								}
							}

							if(isMyFile)
							{

								if(reload)
									clearTimeout(reload);

								if(e == 'add' || e == 'change')
								{
									var toy = require('./toyasm/build/Release/toyasm');
									toy.create(p);
								}

								reload = setTimeout(function() {
									app.files.search();
									$scope.$apply();
								}, 500);
							}
						}
					}));
				}
			}
			setTimeout(function() {
				isLoading = false;
			}, 500);
		};

		app.files.apply = function() {
			$scope.$apply();
		}

		app.files.recursiveSearch = function(folder) {
			if(folder)
			{
				var files = fs.readdirSync(folder);
				for(var f in files)
				{
					var file = folder + '\\' + files[f];
					var stat = fs.statSync(file);
					if(stat && stat.isFile())
					{
						for(var e in app.exts)
						{
							if(files[f].indexOf(app.exts[e].name) > 0 && files[f].indexOf(app.exts[e].name) == files[f].length - app.exts[e].name.length)
							{
								if(file)
									$scope.files.push({path: file, name: files[f]});
							}
						}
					}
					else if(stat && stat.isDirectory())
					{
						if(file)
						{
							app.files.recursiveSearch(file);
						}
					}
				}
			}
		};
	}])

	.controller('FoldersController', ['$scope', function($scope) {
		$scope.folders = app.folders;

		app.drop = function(folder) {
			if(!$scope.folders.ofind(function(i) { return i.path == folder; }))
			{
				$scope.folders.push({path: folder});
				app.folders = $scope.folders;
				save();
				app.files.search();
				$scope.$apply();
			}
		};

		$scope.delete = function(i) {
			$scope.folders.splice(i, 1);
			save();
			app.files.search();
		};
	}])

	.controller('app', ['$scope', function($scope) {
		var isMaximized = false;
		var gui = require('nw.gui');
		var win = gui.Window.get();
		win.setMaximumSize(0, screen.availHeight);

		win.on('maximize', function() {
			isMaximized = true;
		});
		win.on('unmaximize', function() {
			isMaximized = false;
		});

		$scope.close = function() {
			win.close();
		};

		$scope.minimize = function() {
			win.minimize();
		}
	}]);

$(function(){
	var path = './';

	app.files.search();
	app.files.apply();
	$('.loader').delay(700).fadeOut(200, function() {
		$('.main').fadeIn(200);
	});

	$('#dropzone').on('dragover', function(e) {
		e.preventDefault();
		e.stopPropagation();
		$(this).addClass('dragover');
	});

	$('.nav-btn').on('click', function() {
		if($(this).attr('goto') != page)
		{
			var btn = $(this);
			btn.addClass('current');
			$('.nav-btn.'+page).removeClass('current');
			$('#' + page + '_page').fadeOut(200, function() {
				$('#' + btn.attr('goto') + '_page').fadeIn(200);
			});
			page = btn.attr('goto');
		}
	});

	window.addEventListener("drop", function(e) {
		e = e || event;
		e.preventDefault();
	}, false);

	window.addEventListener("dragover", function(e) {
		e = e || event;
		e.preventDefault();
	}, false);

	document.getElementById('dropzone').addEventListener("drop", function (e) {
	  e.preventDefault();
		e.stopPropagation();
		$(this).removeClass('dragover');

		for(var i = 0; i < e.dataTransfer.files.length; i++)
		{
			if(page == pages[0] && e.dataTransfer.items[i].webkitGetAsEntry().isDirectory)
			{
					if(app.drop != null)
						app.drop(e.dataTransfer.files[i].path);
			}
		}

	  return false;
	});

	$('#dropzone').on('dragleave', function(e) {
		e.preventDefault();
		e.stopPropagation();
		$(this).removeClass('dragover');
	});
});
