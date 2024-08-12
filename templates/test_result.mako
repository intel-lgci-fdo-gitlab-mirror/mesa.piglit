<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>${testname} - Details</title>
    <link rel="stylesheet" href="${css}">
  % if value.images:
    <script>module = {}</script>
    <script src="https://unpkg.com/pixelmatch@5.3.0"></script>
    <script>
      var diff_complete = false;

      function readImageData(imageID) {
        var image = document.getElementById(imageID);
        var canvas = document.createElement("canvas");
        canvas.width = image.naturalWidth;
        canvas.height = image.naturalHeight;
        canvas.getContext("2d").drawImage(image, 0, 0);
        return canvas.getContext("2d").getImageData(0, 0, image.naturalWidth, image.naturalHeight);
      }

      function show_image(src) {
        document.getElementById("diffOutput").innerHTML = '<img src="' + src + '">'
      }

      function compare_images() {
        var diff = document.createElement("canvas");
        var ref = document.getElementById("refimg");
        var rend = document.getElementById("rendimg");

        if(!ref.complete || !rend.complete || diff_complete) {
          return;
        }

        var width = ref.naturalWidth
        var height = ref.naturalHeight
        diff.width = width
        diff.height = height

        const ref_data = readImageData("refimg")

        const rend_data = readImageData("rendimg")

        diff_ctx = diff.getContext('2d');
        const diff_data = diff_ctx.createImageData(width, height)

        const diff_count_rel = pixelmatch(ref_data.data, rend_data.data, diff_data.data, width, height, {threshold: 0.1})
        const diff_count_abs = pixelmatch(ref_data.data, rend_data.data, diff_data.data, width, height, {threshold: 0.0, includeAA: true})
        document.getElementById("diffPixelCount").innerHTML = "Different pixels: " + diff_count_abs + " <small>(no tolerance)</small>, <strong>" + diff_count_rel + "</strong> <small>(1% tol.)</small>"

        diff_ctx.putImageData(diff_data, 0, 0)
        diff_complete = true

        document.getElementById("diffimg").src = diff.toDataURL();
      }
    </script>
  % endif
  </head>
  <body>
    <h1>Results for ${testname}</h1>
    <h2>Overview</h2>
    <div>
      <p><b>Result:</b> ${value.result}</p>
    </div>
    <p><a href="${index}">Back to summary</a></p>
    <h2>Details</h2>
    <table>
      <tr>
        <th>Detail</th>
        <th>Value</th>
      </tr>
      <tr>
        <td>Returncode</td>
        <td>${value.returncode}</td>
      </tr>
      <tr>
        <td>Time</td>
        <td>${value.time.delta}</td>
      </tr>
    % if value.images:
      <tr>
        <td>Images</td>
        <td>
          <table>
            <tr>
              <td id="diffPixelCount">Calculating the difference...</td>
              <td>reference</td>
              <td>rendered</td>
            </tr>
          % for image in value.images:
            <tr>
              <td><img width="380px" id="diffimg" onclick="show_image(this.src)"/></td>
              <td><img src="file://${image['image_ref'] if 'image_ref' in image else None}" id="refimg" width="380px" onload="compare_images()" onclick="show_image(this.src)" crossorigin="Anonymous"/></td>
              <td><img src="file://${image['image_render'] if 'image_render' in image else None}" id="rendimg" width="380px" onload="compare_images()" onclick="show_image(this.src)" crossorigin="Anonymous"/></td>
            </tr>
	    <tr><td colspan="3" id="diffOutput"></td></tr>
          % endfor
          </table>
        </td>
      </tr>
    % endif
      <tr>
        <td>Stdout</td>
        <td>
          <pre>${value.out | h}</pre>
        </td>
      </tr>
      <tr>
        <td>Stderr</td>
        <td>
          <pre>${value.err | h}</pre>
        </td>
      </tr>
    % if value.environment:
      <tr>
        <td>Environment</td>
        <td>
          <pre>${value.environment | h}</pre>
        </td>
      </tr>
    % endif
      <tr>
        <td>Command</td>
        <td>
          <pre>${value.command}</pre>
        </td>
      </tr>
    % if value.exception:
      <tr>
        <td>Exception</td>
        <td>
          <pre>${value.exception | h}</pre>
        </td>
      </tr>
    % endif
    % if value.traceback:
      <tr>
        <td>Traceback</td>
        <td>
          <pre>${value.traceback | h}</pre>
        </td>
      </tr>
    % endif
      <tr>
        <td>dmesg</td>
        <td>
          <pre>${value.dmesg | h}</pre>
        </td>
      </tr>
    </table>
    <p><a href="${index}">Back to summary</a></p>
  </body>
</html>
