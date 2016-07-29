(local transform)
(local rigidbody)
(local start (fun (do
	(set transform ((entity'require-transform)))
	((transform'move) (vec 0 0 2))
	(set rigidbody ((entity'require-rigidbody)))
	((((entity'add-sprite))'load) "spritesheet.png")
	(set character-collider ((entity'add-collider)))
	((character-collider'box) (vec 1 1 1))
)))
(local update (fun (do
	(local movement (* delta 16))
	(if (key-pressed UP) 		((rigidbody'addspeed) (vec 0 movement 0)))
	(if (key-pressed LEFT) 	((rigidbody'addspeed) (vec (- movement) 0 0)))
	(if (key-pressed DOWN)	((rigidbody'addspeed) (vec 0 (- movement) 0)))
	(if (key-pressed RIGHT) ((rigidbody'addspeed) (vec movement 0 0)))
	(if (key-pressed SPACE) ((rigidbody'addspeed) (vec 0 0 movement)))
)))
