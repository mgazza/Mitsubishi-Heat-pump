// including plugins
var gulp = require('gulp')
, minifyHtml = require("gulp-minify-html")
, minifyCss = require('gulp-clean-css')
, uglify = require("gulp-uglify")
, concat = require('gulp-concat')
, gzip = require('gulp-gzip')
, inline = require('gulp-inline');

// concat-css
gulp.task('concat-css', function () {
    return gulp.src('./src/src/css/*.css')
	.pipe(concat('app.css'))
	.pipe(gulp.dest('./src/css'));
});
// concat-js
gulp.task('concat-js', function () {
    return gulp.src('./src/src/scripts/*.js')
    .pipe(uglify())
	.pipe(concat('app.js'))
	.pipe(gulp.dest('./src/scripts'));
});


gulp.task('build',['concat-js','concat-css'], function () {
 
});

//dist
gulp.task('dist', function(){
	gulp.src('./src/index.html')
	  .pipe(inline({
		base: './src/',
		js: uglify,
        disabledTypes: ['svg', 'img', 'css']
	  }))
	  .pipe(minifyHtml())
	  //.pipe(gzip())
	  .pipe(gulp.dest('./data/'));
	  
	gulp.src('./src/css/app.css')
	//.pipe(gzip())
	  .pipe(gulp.dest('./data/css/'));
	  
	gulp.src('./src/images/favicon.png')
	  .pipe(gulp.dest('./data/images/'));
	  
	gulp.src('./src/images/icons.svg')
	  //.pipe(gzip())
	  .pipe(gulp.dest('./data/images/'));
	  
});