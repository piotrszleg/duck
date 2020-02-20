// basically if one dummy is hovered over with mouse it will highlight the rest
addEventListener("load", ()=>{
    const dummies=new Map()
    const addHighlight=el=>el.classList.add("selected")
    const removeHighlight=el=>el.classList.remove("selected")
    document.querySelectorAll(".dummy").forEach(el=>{
        const content=el.innerHTML
        if(content=="any"){
            // "any" means that any object can be at the position
            // and to emphasize that rest of "any" dummies aren't highlighted
            el.addEventListener("mouseenter", ()=>addHighlight(el))
            el.addEventListener("mouseleave", ()=>removeHighlight(el))
        } else {
            if(dummies.has(content)){
                dummies.get(content).push(el)
            } else {
                dummies.set(content, [el])
            }
            el.addEventListener("mouseenter", ()=>
                dummies.get(content).forEach(addHighlight))
            el.addEventListener("mouseleave", ()=>
                dummies.get(content).forEach(removeHighlight))
        }
    })
})