import json

def make_schema_obj(o:dict) -> str:
	arr = []
	for key in o:
		if type(o[key]) is bool:
			arr.append("const_json::Bool<\"%s\">" % key)
		elif type(o[key]) is int:
			arr.append("const_json::Int<\"%s\">" % key)
		elif type(o[key]) is float:
			arr.append("const_json::Double<\"%s\">" % key)
		elif type(o[key]) is None:
			arr.append("const_json::Null<\"%s\">" % key)
		elif type(o[key]) is str:
			arr.append("const_json::String<\"%s\">" % key)
		elif type(o[key]) is dict:
			try:
				s = make_schema_obj(o[key])
				s = s[len("const_json::Object_<"):]
				arr.append("const_json::Object<\"%s\",\n\t %s" % (key, s))
			except RuntimeError as e:
				if str(e) == "empty object":
					print("Skipping empty object")
				else:
					raise e
		elif type(o[key]) is list:
			try:
				s = make_schema_arr(o[key])
				s = s[len("const_json::Array_<"):]
				arr.append("const_json::Array<\"%s\",\n\t %s" % (key, s))
			except RuntimeError as e:
				if str(e) == "empty array":
					print("Skipping empty array")
				else:
					raise e
		else:
			raise RuntimeError("uhhhh, but object")

		if len(arr) == 0:
			raise RuntimeError("empty object")
	return "const_json::Object_<%s>" % ",\n\t".join(arr)

def make_schema_arr(a:list) -> str:
	arr = []
	for e in a:
		if type(e) is bool:
			arr.append("const_json::Bool_")
		elif type(e) is int:
			arr.append("const_json::Int_")
		elif type(e) is float:
			arr.append("const_json::Double_")
		elif type(e) is None:
			arr.append("const_json::Null_")
		elif type(e) is str:
			arr.append("const_json::String_")
		elif type(e) is dict:
			try:
				arr.append(make_schema_obj(e))
			except RuntimeError as ex:
				if str(ex) == "empty object":
					print("Skipping empty object")
				else:
					raise ex
		elif type(e) is list:
			try:
				arr.append(make_schema_arr(e))
			except RuntimeError as ex:
				if str(ex) == "empty array":
					print("Skipping empty array")
				else:
					raise ex
		else:
			raise RuntimeError("uhhhh, but array")
		if len(arr) == 0:
			raise RuntimeError("empty array")
	return "const_json::Array_<%s>" % ",\n\t".join(arr)

def make_schema(s:str, pretty:bool=False) -> str:
	parsed = json.loads(s)
	out = ""
	if type(parsed) is dict:
		out = make_schema_obj(parsed)
	elif type(parsed) is arr:
		out = make_schema_arr(parsed)
	else:
		raise RuntimeError("BadType")

	if not pretty:
		return out

