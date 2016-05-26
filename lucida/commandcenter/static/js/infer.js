function infer_submit() {
  var fd = new FormData(this);
  console.log('$$$$$$$$$$' + $('form').serialize())
  var formData = new FormData($(this)[0]);
  console.log('$$$$$$$$$$' + formData.op)
  $.ajax({
    url: "/infer",
    type: "POST",
    data: $('form').serialize(),
    success: function(response) {
      console.log(response);
    }, error: function(error) {
      console.log(error);
    }
  });  
}


